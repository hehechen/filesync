#include "syncserver.h"
#include "sysutil.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <functional>
#include <boost/bind.hpp>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <fstream>

SyncServer::SyncServer(const char *root,muduo::net::EventLoop *loop,int port)
    : rootDir(root),
      serverAddr(port),
      server_(loop,serverAddr,"syncServer")
{
    server_.setConnectionCallback(boost::bind(&SyncServer::onConnection,this,_1));
    server_.setMessageCallback(boost::bind(&SyncServer::onMessage,this,_1,_2,_3));

    codec.registerCallback<filesync::SyncInfo>(std::bind(&SyncServer::onSyncInfo,this,
                                                         std::placeholders::_1,std::placeholders::_2));
    codec.registerCallback<filesync::FileInfo>(std::bind(&SyncServer::onFileInfo,this,
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
    {//此ip第一个连接的socket设置为控制通道
        Con_Vec vec;
        vec.push_back(conn);
        ipMaps.insert(make_pair(ip,std::move(vec)));
        info_ptr->type = CONTROL;
        conn->setContext(info_ptr);
        //为此客户端初始化发送队列
        TaskQueue emptyQueue;
        queueMaps.insert(make_pair(ip,emptyQueue));
        //告知客户端这是控制通道
        filesync::IsControl msg;
        msg.set_id(1);
        std::string send_str = Codec::enCode(msg);
        conn->send(send_str);
    }
}
//接收到原始的数据包后的处理,根据conn的不同采取不同的处理
void SyncServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,muduo::Timestamp receiveTime)
{
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    if(info_ptr->type == CONTROL)
    {//控制通道
        CHEN_LOG(DEBUG,"cmd parse...");
        codec.parse(conn,buf);
    }
    else
    {//文件传输通道有数据到来，判断是消息还是文件数据
        if(info_ptr->isRecving)
        {//是文件数据
            recvFile(conn,info_ptr);
        }

        else
        {//解析信息并调用onFileInfo
            CHEN_LOG(DEBUG,"file parse...");
            codec.parse(conn,buf);
        }
    }
}
//客户端第一次上线发过来文件信息
void SyncServer::onInit(muduo::net::TcpConnectionPtr &conn, InitPtr message)
{
    for(int i=0;i<message->info_size();i++)
    {
        filesync::SyncInfo info = message->info(i);
        std::string filename = info.filename();
        std::string localname = rootDir+filename;
        SyncInfoPtr tmpPtr(&info);
        onSyncInfo(conn,tmpPtr);
    }
    syncToClient(conn);
}
//收到客户端发来的修改信息
void SyncServer::onSyncInfo(const muduo::net::TcpConnectionPtr &conn,
                            const SyncInfoPtr &message)
{
    int id = message->id();
    std::string filename = message->filename();
    std::string localname = rootDir+filename;
    switch(id)
    {
    case 0:
    {//创建文件夹
        if(::access(localname.c_str(),F_OK) != 0)
            mkdir(localname.c_str(),0666);
        break;
    }
    case 1:
    {//创建文件
    }
    case 2:
    {//文件内容修改
        //向客户端发送sendfile命令
        filesync::SendFile msg;
        msg.set_id(1);
        msg.set_filename(filename);
        std::string cmd = codec.enCode(msg);
        CHEN_LOG(DEBUG,"sendfile :----%s",cmd.c_str());
        conn->send(cmd);
        break;
    }
    case 3:
    {//删除文件
        if(access(localname.c_str(),F_OK) != 0) //文件不存在
            CHEN_LOG(INFO,"file %s don't exit",localname.c_str());
        else
        {
            char rmCmd[512];
            sprintf(rmCmd,"rm -rf %s",localname.c_str());
            system(rmCmd);
        }
        break;
    }
    case 4:
    {//重命名
        std::string newFilename = message->newfilename();
        if(rename(localname.c_str(),(rootDir+newFilename).c_str()) < 0)
            CHEN_LOG(ERROR,"rename %s error",localname.c_str());
        CHEN_LOG(DEBUG,"rename %s to %s",localname.c_str(),
                                    (rootDir+newFilename).c_str());
    }
    }
}
//解析fileInfo消息后的回调函数
void SyncServer::onFileInfo(const muduo::net::TcpConnectionPtr &conn,
                            const FileInfoPtr &message)
{
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    info_ptr->isRecving = true;
    std::string filename = info_ptr->filename = rootDir+message->filename();
    info_ptr->totalSize = info_ptr->remainSize = message->size();
    //如果同名文件存在则删除
    if(access(filename.c_str(),F_OK) == 0)
        if(::remove(filename.c_str()) < 0)
            CHEN_LOG(ERROR,"remove error");
    recvFile(conn,info_ptr);
}

/**
 * @brief SyncServer::recvFile  从Buffer中接收文件数据，接收完发送给其它客户端
 * @param info_ptr
 * @param inputBuffer
 */
void SyncServer::recvFile(const muduo::net::TcpConnectionPtr &conn,Info_ConnPtr &info_ptr)
{
    int len = conn->inputBuffer()->readableBytes();
    if(len >= info_ptr->remainSize)
    {//文件接收完
        sysutil::fileRecvfromBuf(info_ptr->filename.c_str(),
                                 conn->inputBuffer()->peek(),info_ptr->remainSize);
        conn->inputBuffer()->retrieve(info_ptr->remainSize);
        info_ptr->isRecving = false;
        info_ptr->remainSize = 0;
        info_ptr->totalSize = 0;
        //发送给其它客户端
        for(auto it = queueMaps.begin();it!=queueMaps.end();++it)
        {
            if(it->first == conn->peerAddress().toIp())
                continue;
            muduo::net::TcpConnectionPtr idleConn = getIdleConn(it->first);
            if(idleConn == nullptr)
            {
                it->second.push(std::bind(&SyncServer::sendfileWithproto,this,
                                          std::placeholders::_1,info_ptr->filename));
            }
            sendfileWithproto(idleConn,info_ptr->filename);
        }
        info_ptr->filename.clear();
    }
    else
    {
        sysutil::fileRecvfromBuf(info_ptr->filename.c_str(),
                                 conn->inputBuffer()->peek(),len);
        conn->inputBuffer()->retrieve(len);
        info_ptr->remainSize -= len;
    }
}


/**
 * @brief sendfileWithproto 先发送fileInfo信息再发送文件内容，不能发送文件夹！！
 *        服务端不能和客户端一样阻塞写，要用网络库的接口，socket可写的时候调用回调函数写数据
 * @param conn  此时conn已有context
 * @param localname 本地路径
 * @param remotename    不包含顶层文件夹的文件名
 */
void SyncServer::sendfileWithproto(const muduo::net::TcpConnectionPtr &conn
                                   ,std::string &localname)
{
    int fd = open(localname.c_str(),O_RDONLY);
    if(-1 == fd)
    {
        CHEN_LOG(INFO,"open file %s error",localname);
        return;
    }
    struct stat sbuf;
    fstat(fd,&sbuf);
    if (!S_ISREG(sbuf.st_mode))
    {//不是一个普通文件
        CHEN_LOG(WARN,"failed to open file");
        return;
    }
    int bytes_to_send = sbuf.st_size; //  文件大小
    close(fd);
    FILE *fp = fopen(localname.c_str(),"rb");   //以二进制读取
    if(fp == NULL)
        CHEN_LOG(ERROR,"open file %s error",localname.c_str());
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    info_ptr->isIdle = false;
    info_ptr->fp = fp;
    conn->setWriteCompleteCallback(boost::bind(&SyncServer::onWriteComplete,this,conn));
    //先发送fileInfo信息
    filesync::FileInfo msg;
    msg.set_size(bytes_to_send);
    msg.set_filename(localname.substr(rootDir.size()));
    conn->send(Codec::enCode(msg));
}

/**
 * @brief onWriteComplete   fileInfo信息发送完或一块文件发送完执行此函数，
 *                          此函数仅在sendfileWithproto函数中被注册，文件发送完后执行下一个发送任务
 * @param conn              用此conn发送
 */
void SyncServer::onWriteComplete(const muduo::net::TcpConnectionPtr &conn)
{
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    FILE* fp = info_ptr->fp;
    char buf[64*1024];          //一次发64k
    size_t nread = ::fread(buf, 1, sizeof buf, fp);
    if (nread > 0)
    {
      conn->send(buf, static_cast<int>(nread));
    }
    else
    {
      Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
      info_ptr->fp = NULL;
      info_ptr->isIdle = true;
      conn->setWriteCompleteCallback(NULL);
      doNextSend(conn);     //用此连接执行下一个发送任务
    }
}
/**
 * @brief SyncServer::getIdleConn  获取该客户端的空闲文件传输连接
 * @param ip                       该客户端的ip
 * @return
 */
muduo::net::TcpConnectionPtr SyncServer::getIdleConn(muduo::string ip)
{
    Con_Vec conVec = ipMaps[ip];
    for(auto it:conVec)
    {
        Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(it->getContext());
        if(info_ptr->type == ConType::CONTROL)
            continue;
        if(info_ptr->isIdle)
            return it;
    }
    return nullptr;
}
/**
 * @brief SyncServer::doNextSend    从发送队列取出任务执行
 * @param conn
 */
void SyncServer::doNextSend(const muduo::net::TcpConnectionPtr &conn)
{
    auto it = queueMaps.find(conn->peerAddress().toIp());
    if(it != queueMaps.end())
    {
        if(!it->second.empty())
        {
            SendTask task = it->second.front();
            it->second.pop();
            task(conn);
        }
    }
    else
        CHEN_LOG(ERROR,"can't find send queue");
}

