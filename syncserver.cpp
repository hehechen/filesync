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

#include <QFile>
#include <QCryptographicHash>
#include <QByteArray>

using namespace std;

SyncServer::SyncServer(muduo::net::EventLoop *loop,int port)
    : serverAddr(port),
      server_(loop,serverAddr,"syncServer"),
      pc(ParseConfig::getInstance())
{
    pc->loadfile();
    rootDir = pc->getSyncRoot();
    initMd5(rootDir);
    CHEN_LOG(DEBUG,"MD5 completed");
    server_.setConnectionCallback(boost::bind(&SyncServer::onConnection,this,_1));
    server_.setMessageCallback(boost::bind(&SyncServer::onMessage,this,_1,_2,_3));

    codec.registerCallback<filesync::SignIn>(std::bind(&SyncServer::onSignIn,this,
                                                       std::placeholders::_1,std::placeholders::_2));
    codec.registerCallback<filesync::Init>(std::bind(&SyncServer::onInit,this,
                                                     std::placeholders::_1,std::placeholders::_2));
    codec.registerCallback<filesync::SyncInfo>(std::bind(&SyncServer::onSyncInfo,this,
                                                         std::placeholders::_1,std::placeholders::_2));
    codec.registerCallback<filesync::FileInfo>(std::bind(&SyncServer::onFileInfo,this,
                                                         std::placeholders::_1,std::placeholders::_2));
}
//接收到连接，初始化conn相关的数据结构
void SyncServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if(conn->connected())
    {
        Info_ConnPtr info_ptr(new Info_Conn );
        conn->setContext(info_ptr);
    }
    else if(conn->disconnected())
    {//连接断开
        Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
        auto it_userMaps = userMaps.find(info_ptr->username);
        if(it_userMaps != userMaps.end())
            userMaps.erase(it_userMaps);
        auto it_queueMaps = sendListMaps.find(info_ptr->username);
        if(it_queueMaps != sendListMaps.end())
            sendListMaps.erase(it_queueMaps);
    }
}
//接收到原始的数据包后的处理
void SyncServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,muduo::Timestamp receiveTime)
{
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    if(info_ptr->isRecving)
    {//是文件数据
        recvFile(conn,info_ptr);
    }

    else
    {//是proto中定义的消息
        codec.parse(conn,buf);
    }
}
//用户登录时，更新userMap，并根据此用户的连接次数设置此conn的类型
void SyncServer::onSignIn(const muduo::net::TcpConnectionPtr &conn, const SignInPtr &message)
{
    string username = message->username();
    string password = message->password();
    if(!pc->checkUser(username,password))
    {//帐号错误直接关闭连接
        CHEN_LOG(INFO,"user invalid");
        conn->forceClose();
        return;
    }
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    info_ptr->username = username;
    auto it = userMaps.find(username);
    if(it != userMaps.end())
    {
        it->second.push_back(conn);
        info_ptr->type = ConType::DATA;
    }
    else
    {//此用户第一个连接的socket设置为控制通道
        info_ptr->type = CONTROL;
        Con_Vec vec;
        vec.push_back(conn);
        {
            muduo::MutexLockGuard mutexLock(userMaps_mutex);
            userMaps.insert(make_pair(username,std::move(vec)));
        }
        //为此客户端初始化发送队列
        TaskList emptyQueue;
        {
            muduo::MutexLockGuard mutexLock(sendListMaps_mutex);
            sendListMaps.insert(make_pair(username,emptyQueue));
        }
        //告知客户端这是控制通道
        filesync::IsControl msg;
        msg.set_id(1);
        std::string send_str = Codec::enCode(msg);
        conn->send(send_str);
    }
}


//客户端第一次上线发过来文件信息
void SyncServer::onInit(const muduo::net::TcpConnectionPtr &conn,const InitPtr &message)
{

    CHEN_LOG(DEBUG,"INIT RECEIVE");
    vector<string> clientFiles;
    for(int i=0;i<message->info_size();i++)
    {
        filesync::SyncInfo info = message->info(i);
        string filename = info.filename();
        string localname = rootDir+filename;
        SyncInfoPtr tmpPtr(new filesync::SyncInfo);
        tmpPtr->set_filename(filename);
        tmpPtr->set_md5(info.md5());
        tmpPtr->set_id(info.id());
        onSyncInfo(conn,tmpPtr);
        clientFiles.push_back(localname);
    }
    syncToClient(conn,rootDir,clientFiles);
}
//收到客户端发来的修改信息
void SyncServer::onSyncInfo(const muduo::net::TcpConnectionPtr &conn,
                            const SyncInfoPtr &message)
{
    int id = message->id();
    string filename = message->filename();
    string newFilename = message->newfilename();
    string localname = rootDir+filename;
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
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
        {
            muduo::MutexLockGuard   mutexLock(md5Maps_mutex);
            auto it = md5Maps.find(localname);
            if(it != md5Maps.end())
                if(it->second == message->md5())
                {
                    break;      //已有相同文件则跳过
                }
            md5Maps[localname] = message->md5();
        }
        //向客户端发送sendfile命令
        filesync::SendFile msg;
        msg.set_id(1);
        msg.set_filename(filename);
        string cmd = codec.enCode(msg);
        CHEN_LOG(INFO,"command sendfile :----%s",filename.c_str());
        conn->send(cmd);
        break;
    }
    case 3:
    {//删除文件
        {
            muduo::MutexLockGuard mutexLock(md5Maps_mutex);
            auto it = md5Maps.find(localname);
            if(it != md5Maps.end())
                md5Maps.erase(it);        //删除此文件的md5信息
        }
        if(access(localname.c_str(),F_OK) != 0) //文件不存在
            CHEN_LOG(INFO,"file %s don't exit",localname.c_str());
        else
        {
            CHEN_LOG(INFO,"remove file %s",localname.c_str());
            char rmCmd[512];
            sprintf(rmCmd,"rm -rf %s",localname.c_str());
            system(rmCmd);
        }
        int sendsize = message->size();
        if(sendsize > 0)
        {//客户端上传过程中文件被删除
            int pos = message->filename().find_last_of('/');
            string parDir = message->filename().substr(0,pos+1);
            string subfile = message->filename().substr(pos+1);
            string filename = rootDir+parDir+"."+ info_ptr->username.c_str()+subfile;
            Con_Vec conVec = userMaps[info_ptr->username];
            for(auto it = conVec.begin();it != conVec.end();++it)
            {
                Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>((*it)->getContext());
                if(info_ptr->receiveFilename == filename && info_ptr->isRecving)
                {
                    info_ptr->remainSize = sendsize-(info_ptr->totalSize-info_ptr->remainSize);
                    info_ptr->totalSize = sendsize;
                    info_ptr->isRemoved_receving = true;
                }
            }
        }
        handleRemove_sending(localname);

        break;
    }
    case 4:
    {//重命名
        if(rename(localname.c_str(),(rootDir+newFilename).c_str()) < 0)
            CHEN_LOG(WARN,"rename %s error",localname.c_str());
        CHEN_LOG(DEBUG,"rename %s to %s",localname.c_str(),
                 (rootDir+newFilename).c_str());
        break;
    }
    }
    if(id!=1 && id!=2)
    {//发送给其它客户端
        {
            muduo::MutexLockGuard mutexLock(userMaps_mutex);
            for(auto it = userMaps.begin();it!=userMaps.end();++it)
            {
                if(it->first == info_ptr->username)
                    continue;
                sysutil::send_SyncInfo(it->second[0],id,filename,newFilename);
            }
        }
    }
}
//解析fileInfo消息后的回调函数
void SyncServer::onFileInfo(const muduo::net::TcpConnectionPtr &conn,
                            const FileInfoPtr &message)
{
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    info_ptr->isRecving = true;
    //文件接收时以隐藏文件存储，隐藏文件名为XXX/.[username][filename]
    int pos = message->filename().find_last_of('/');
    string parDir = message->filename().substr(0,pos+1);
    string subfile = message->filename().substr(pos+1);
    string filename = info_ptr->receiveFilename = rootDir+parDir+"."+
            info_ptr->username.c_str()+subfile;
    CHEN_LOG(INFO,"temp file :%s",filename.c_str());
    info_ptr->totalSize = info_ptr->remainSize = message->size();
    //如果同名文件存在则删除
    if(access(filename.c_str(),F_OK) == 0)
        if(::remove(filename.c_str()) < 0)
            CHEN_LOG(ERROR,"remove error");
    recvFile(conn,info_ptr);
}

//客户端接收过程中文件被删除
void SyncServer::handleRemove_sending(string localname)
{
    Con_Vec conVec = sendfileMaps[localname];
    if(!conVec.empty())
    {
        for(auto it:conVec)
        {
            Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(it->getContext());
            if(!info_ptr->isIdle)
            {
                info_ptr->isRemoved_sending = true;
            }
            //从待发送列表中删除
            auto it_map = sendListMaps.find(info_ptr->username);
            for(auto it = it_map->second.begin();it!=it_map->second.end();)
            {
                if(it->first == localname)
                    it_map->second.erase(it);
                else
                    ++it;
            }
        }
    }
}

/**
 * @brief SyncServer::recvFile  从Buffer中接收文件数据，接收完发送给其它客户端
 * @param conn
 * @param info_ptr
 */
void SyncServer::recvFile(const muduo::net::TcpConnectionPtr &conn,Info_ConnPtr &info_ptr)
{
    int len = conn->inputBuffer()->readableBytes();
    if(len >= info_ptr->remainSize)
    {//文件接收完
        sysutil::fileRecvfromBuf(info_ptr->receiveFilename.c_str(),
                                 conn->inputBuffer()->peek(),info_ptr->remainSize);
        conn->inputBuffer()->retrieve(info_ptr->remainSize);
        info_ptr->isRecving = false;
        info_ptr->remainSize = 0;
        info_ptr->totalSize = 0;
        if(info_ptr->isRemoved_receving)
            remove(info_ptr->receiveFilename.c_str());
        else
        {
            //将原文件删除，隐藏文件改名
            int pos = info_ptr->receiveFilename.find_last_of('/');
            string parDir = info_ptr->receiveFilename.substr(0,pos+1);
            string subfile = info_ptr->receiveFilename.substr(pos+1);
            string filename = parDir+subfile.substr(info_ptr->username.size()+1);
            if(access(filename.c_str(),F_OK) == 0)
                if(::remove(filename.c_str()) < 0)
                    CHEN_LOG(ERROR,"remove error");
            if(rename(info_ptr->receiveFilename.c_str(),filename.c_str()) < 0)
                CHEN_LOG(ERROR,"rename %s error",info_ptr->receiveFilename.c_str());
            CHEN_LOG(INFO,"receive file COMPLETED:%s",filename.c_str());
            sendListMaps_mutex.lock();
            //发送给其它客户端
            handleRemove_sending(filename);//将正在发送的同名文件取消发送
            for(auto it = sendListMaps.begin();it!=sendListMaps.end();++it)
            {
                if(it->first == info_ptr->username)
                {
                    continue;
                }
                muduo::net::TcpConnectionPtr idleConn = getIdleConn(it->first);
                if(idleConn == nullptr)
                {//加入发送队列
                    {
                        it->second.push_back({filename,std::bind(&SyncServer::sendfileWithproto,this,
                                              std::placeholders::_1,filename)
                                             });
                        sendListMaps_mutex.unlock();
                    }
                }
                else
                {
                    sendListMaps_mutex.unlock();
                    sendfileWithproto(idleConn,filename);
                }
            }
            sendListMaps_mutex.unlock();
        }
        info_ptr->receiveFilename.clear();
        info_ptr->md5.clear();
        info_ptr->isRemoved_receving = false;
        //如果buffer还有数据则继续解析
        if(conn->inputBuffer()->readableBytes() > 0)
            onMessage(conn,conn->inputBuffer(),muduo::Timestamp::now());
    }
    else
    {
        sysutil::fileRecvfromBuf(info_ptr->receiveFilename.c_str(),
                                 conn->inputBuffer()->peek(),len);
        conn->inputBuffer()->retrieve(len);
        info_ptr->remainSize -= len;
    }
}


/**
 * @brief sendfileWithproto 先发送fileInfo信息再发送文件内容，不能发送文件夹！！
 *        服务端不能和客户端一样阻塞写，要用网络库的接口，socket可写的时候调用回调函数写数据
 * @param conn  文件由此conn发出，此时conn已有context
 * @param localname 本地路径
 */
void SyncServer::sendfileWithproto(const muduo::net::TcpConnectionPtr &conn
                                   ,string &localname)
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
    info_ptr->sendFilename = localname;
    sendfileMaps[localname].push_back(conn);
    conn->setWriteCompleteCallback(boost::bind(&SyncServer::onWriteComplete,this,conn));
    //先发送fileInfo信息
    filesync::FileInfo msg;
    msg.set_size(bytes_to_send);
    msg.set_filename(localname.substr(rootDir.size()));
    conn->send(Codec::enCode(msg));
    CHEN_LOG(INFO,"SEND FILEINFO :%s",localname.c_str());
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
    size_t nread = -1;
    char buf[64*1024];          //一次发64k
    if(!info_ptr->isRemoved_sending)
        nread = ::fread(buf, 1, sizeof buf, fp);
    if(nread <=0 || info_ptr->isRemoved_sending)
    {
        CHEN_LOG(INFO,"SEND FILE completed:%s",info_ptr->sendFilename.c_str());
        if(info_ptr->isRemoved_sending)
        {//说明该文件已被删除，告知客户端已发送的大小
            sysutil::send_SyncInfo(userMaps[info_ptr->username][0],3,
                    info_ptr->sendFilename.substr(rootDir.size()),"",info_ptr->sendSize);
            CHEN_LOG(INFO,"SEND SIZE :%d",info_ptr->sendSize);
            info_ptr->sendFilename.clear();
            info_ptr->sendSize = 0;
        }
        auto it_map = sendfileMaps.find(info_ptr->username);
        if(it_map!=sendfileMaps.end())
        {//从sendfileMaps删除此次发送
            for(auto it_vec = it_map->second.begin();it_vec!=it_map->second.end();)
            {
                if(it_vec->get() == conn.get())
                    it_map->second.erase(it_vec);
                else
                    ++it_vec;
            }
        }
        conn->setWriteCompleteCallback(NULL);
        info_ptr->fp = NULL;
        info_ptr->isIdle = true;
        info_ptr->isRemoved_sending = false;
        doNextSend(conn);     //用此连接执行下一个发送任务
    }
    else if (nread > 0)
    {
        conn->send(buf, static_cast<int>(nread));
        info_ptr->sendSize += nread;
        CHEN_LOG(INFO,"TOTAL NREAD:%d",info_ptr->sendSize);
    }

}
/**
 * @brief SyncServer::getIdleConn  获取该客户端的空闲文件传输连接
 * @param username                       该客户端的username
 * @return
 */
muduo::net::TcpConnectionPtr SyncServer::getIdleConn(string username)
{
    userMaps_mutex.lock();
    Con_Vec conVec = userMaps[username];
    userMaps_mutex.unlock();
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
    Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
    sendListMaps_mutex.lock();
    auto it = sendListMaps.find(info_ptr->username);
    if(it != sendListMaps.end())
    {
        if(!it->second.empty())
        {
            SendTask task = it->second.front().second;
            it->second.pop_front();
            sendListMaps_mutex.unlock();
            task(conn);
        }
        else
            sendListMaps_mutex.unlock();
    }
    else
    {
        sendListMaps_mutex.unlock();
        CHEN_LOG(ERROR,"can't find send queue");
    }
    sendListMaps_mutex.unlock();
}

void SyncServer::syncToClient(const muduo::net::TcpConnectionPtr &conn,string dir,
                              vector<string> &files)
{
    DIR *odir = NULL;
    if((odir = opendir(dir.c_str())) == NULL)
        CHEN_LOG(ERROR,"open dir %s error",dir.c_str());
    struct dirent *dent;
    while((dent = readdir(odir)) != NULL)
    {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;
        string subdir = string(dir) + dent->d_name;
        string remote_subdir = subdir.substr(rootDir.size());
        bool isFound = false;
        for(auto it:files)
        {
            if(it == subdir)
            {
                isFound = true;
                break;
            }
        }
        if(isFound)
            continue;
        if(dent->d_type == DT_DIR)
        {//文件夹
            sysutil::send_SyncInfo(conn,0,remote_subdir);
            syncToClient(conn,(subdir + "/").c_str(),files);
        }
        else
        {
            Info_ConnPtr info_ptr = boost::any_cast<Info_ConnPtr>(conn->getContext());
            string username = info_ptr->username;
            muduo::net::TcpConnectionPtr idleConn = getIdleConn(username);
            if(idleConn == nullptr)
            {//加入发送队列
                muduo::MutexLockGuard mutexLock(sendListMaps_mutex);
                sendListMaps[username].push_back(
                {subdir,std::bind(&SyncServer::sendfileWithproto,this,
                 std::placeholders::_1,subdir)
                            });
            }
            else
                sendfileWithproto(idleConn,subdir);
        }
    }
}
/**
 * @brief SyncServer::initMd5 初始化md5Maps
 */
void SyncServer::initMd5(string dir)
{
    DIR *odir = NULL;
    if((odir = opendir(dir.c_str())) == NULL)
        CHEN_LOG(ERROR,"open dir %s error",dir.c_str());
    struct dirent *dent;
    while((dent = readdir(odir)) != NULL)
    {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;
        string subdir = string(dir) + dent->d_name;
        if(dent->d_type == DT_DIR)
        {//文件夹
            initMd5(subdir + "/");
        }
        else
        {
            string md5 = sysutil::getFileMd5(subdir);
            md5Maps.insert(make_pair(subdir,md5));
        }
    }
}

