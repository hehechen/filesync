#include "syncserver.h"
#include "sysutil.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <functional>
#include <boost/bind.hpp>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <fstream>

SyncServer::SyncServer(muduo::net::EventLoop *loop,int port):serverAddr(port),
    server_(loop,serverAddr,"syncServer")
{
    server_.setConnectionCallback(boost::bind(&SyncServer::onConnection,this,_1));
    server_.setMessageCallback(boost::bind(&SyncServer::onMessage,this,_1,_2,_3));

    codec.registerCallback<filesync::syncInfo>(std::bind(&SyncServer::onSyncInfo,this,
                                                         std::placeholders::_1,std::placeholders::_2));
    codec.registerCallback<filesync::fileInfo>(std::bind(&SyncServer::onFileInfo,this,
                                                         std::placeholders::_1,std::placeholders::_2));
}
//连接到来时，更新ipMap，并根据此ip的连接次数设置此conn的类型
void SyncServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    Info_ConnPtr info_ptr(new Info_Conn );
    muduo::string ip = conn->peerAddress().toIp();
    auto it = ipMaps.find(ip);
    if(it != ipMaps.end())
    {
        it->second.push_back(conn);
        info_ptr->type = ConType::DATA;
        conn->setContext(info_ptr);
    }
    else
    {
        Con_Vec vec;
        vec.push_back(conn);
        ipMaps.insert(make_pair(ip,std::move(vec)));
        info_ptr->type = CONTROL;
        conn->setContext(info_ptr);
    }
}
//接收到原始的数据包后的处理,根据conn的不同采取不同的处理
void SyncServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,muduo::Timestamp receiveTime)
{
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    if(info_ptr->type == CONTROL)
    {//控制通道
        codec.parse(conn,buf);
    }
    else
    {//文件传输通道有数据到来，判断是消息还是文件数据
        if(info_ptr->isRecving)
        {//是文件数据
            recvFile(info_ptr,buf);
        }

        else
        {//解析信息并调用onFileInfo
            codec.parse(conn,buf);
        }
    }
}
//收到客户端发来的修改信息
void SyncServer::onSyncInfo(const muduo::net::TcpConnectionPtr &conn,
                            const syncInfoPtr &message)
{
    int id = message->id();
    string filename = message->filename();
    switch(id)
    {
    case 1:
    {//创建文件
    }
    case 2:
    {//文件内容修改
        //向客户端发送sendfile命令
        filesync::sendfile msg;
        msg.set_id(1);
        msg.set_filename(filename);
        std::string cmd = codec.enCode(msg);
        CHEN_LOG(DEBUG,"syncInfo 1 :----%s",cmd.c_str());
        conn->send(cmd);
        break;
    }
    }
}
//解析fileInfo消息后的回调函数
void SyncServer::onFileInfo(const muduo::net::TcpConnectionPtr &conn,
                            const fileInfoPtr &message)
{
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    info_ptr->isRecving = true;
    info_ptr->filename = std::move(message->filename());
    info_ptr->totalSize = info_ptr->remainSize = message->size();
    recvFile(info_ptr,conn->inputBuffer());
}
/**
 * @brief SyncServer::recvFile  从Buffer中接收文件数据
 * @param info_ptr
 * @param inputBuffer
 */
void SyncServer::recvFile(Info_ConnPtr &info_ptr, muduo::net::Buffer *inputBuffer)
{
    int len = inputBuffer->readableBytes();
    if(len >= info_ptr->remainSize)
    {//文件接受完
        sysutil::fileRecvfromBuf(info_ptr->filename.c_str(),
                                 inputBuffer->peek(),info_ptr->remainSize);
        info_ptr->isRecving = false;
        info_ptr->remainSize = 0;
        info_ptr->totalSize = 0;
        info_ptr->filename.clear();
    }
    else
    {
        sysutil::fileRecvfromBuf(info_ptr->filename.c_str(),
                                 inputBuffer->peek(),len);
        info_ptr->remainSize -= len;
    }
}
