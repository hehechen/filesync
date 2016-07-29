#include "syncserver.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <fstream>

#include "protobuf/filesync.init.pb.h"

SyncServer::SyncServer(muduo::net::EventLoop *loop,int port):serverAddr(port),
    server_(loop,serverAddr,"syncServer")
{
    server_.setConnectionCallback(boost::bind(&SyncServer::onConnection,this,_1));
    server_.setMessageCallback(boost::bind(&SyncServer::onMessage,this,_1,_2,_3));
}
//连接到来时，更新ipMap，并根据此ip的连接次数设置此conn的类型
void SyncServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    struct Info_Conn info;
    muduo::string ip = conn->peerAddress().toIp();
    auto it = ipMaps.find(ip);
    if(it != ipMaps.end())
    {
        int times = it->second.size();
        it->second.push_back(conn);
        info.type = ConType(times+1);
        conn->setContext(info);
    }
    else
    {
        Con_Vec vec;
        vec.push_back(conn);
        ipMaps.insert(make_pair(ip,std::move(vec)));
        info.type = CONTROL;
        conn->setContext(info);
    }
}
//接收到原始的数据包后的处理,根据conn的不同采取不同的处理
void SyncServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                          muduo::net::Buffer *buf,muduo::Timestamp receiveTime)
{
    struct Info_Conn info = boost::any_cast<Info_Conn>(conn->getContext());
    if(info.type == CONTROL)
    {//控制通道
        while (buf->readableBytes() >= KHeaderLen) // kHeaderLen == 4
        {
            const void* data = buf->peek();
            int32_t len = *static_cast<const int32_t*>(data); // SIGBUS
            if (len > 65536 || len < 0)
            {
                LOG_ERROR << "Invalid length " << len;
                conn->shutdown();
                break;
            }
            else if (buf->readableBytes() >= len + KHeaderLen)
            {//该消息已经全部发送过来
                buf->retrieve(KHeaderLen);
                string message(buf->peek(), len);
                onStringMessage(conn,message,receiveTime);
                buf->retrieve(len);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        if(info.filename.empty())
            LOG_ERROR<< "filename don't exit";
        else
        {//接收文件
            if(buf->readableBytes() > info.remain_size)
                LOG_ERROR<< "Invalid length";
            else
            {
                fstream file;
                file.open(info.filename.c_str(),ofstream::out | fstream::app);
                string message(buf->peek(), buf->readableBytes());
                file<<message;
                file.close();
                buf->retrieve(buf->readableBytes());
                info.remain_size -= buf->readableBytes();
            }
        }
    }

}
//解析到来自客户端的信息后根据文件大小选择相应的conn
void SyncServer::onStringMessage(const muduo::net::TcpConnectionPtr &conn,
                                 const string &message, muduo::Timestamp)
{
    filesync::init msg;
    msg.ParseFromString(message);
    int size = msg.size();
    muduo::string ip = conn->peerAddress().toIp();
    auto it = ipMaps.find(ip);
    if(it == ipMaps.end())
        LOG_ERROR<<"IP don't exit";
    Con_Vec desCons = it->second;
    if(desCons.size() !=3 )
        LOG_ERROR<<"connection break"<<endl;
    //根据文件大小选择相应的conn
    struct Info_Conn info;
    if(size >1024*1024*100) //100M
        info = boost::any_cast<Info_Conn>
                (desCons[2]->getContext());
    else
        info = boost::any_cast<Info_Conn>
                (desCons[1]->getContext());
    info.filename = msg.filename();
    info.total_size = msg.size();
    info.remain_size = msg.size();
}
