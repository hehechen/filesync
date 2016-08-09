#ifndef SYNCSERVER_H
#define SYNCSERVER_H
#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <queue>
#include <list>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>

#include "parseconfig.h"
#include "codec.h"
#include "protobuf/filesync.pb.h"

//TcpConnectionPtr的context存放着此conn的类型
enum ConType {  CONTROL=1,DATA };
//TcpConnection的context存放的结构体
struct Info_Conn{
    ConType type;
    std::string username;
    bool isIdle = true;    //是否可发送
    bool isRecving = false; //是否正在接收文件
    bool isRemoved = false; //接收到一半被删除
    int totalSize = 0;      //要接收文件的大小
    int remainSize = 0;        //还未接收的大小
    int sendSize = 0;   //已发送的文件大小
    std::string sendFilename;
    std::string receiveFilename;   //正在接收的文件名
    std::string md5;        //要接收的文件的md5
    FILE *fp;     //要发送的文件的fp
};
typedef std::shared_ptr<Info_Conn> Info_ConnPtr;

//相应protobuf消息的回调函数
typedef std::shared_ptr<filesync::SignIn> SignInPtr;
typedef std::shared_ptr<filesync::Init> InitPtr;
typedef std::shared_ptr<filesync::SyncInfo> SyncInfoPtr;
typedef std::shared_ptr<filesync::FileInfo> FileInfoPtr;

class SyncServer
{
public:
    SyncServer(muduo::net::EventLoop *loop, int port);
    void start()    {   server_.start();    }
private:
    static const int KHeaderLen = 4;    //包的长度信息为4字节

    muduo::net::InetAddress serverAddr;
    muduo::net::TcpServer server_;
    ParseConfig *pc;
    Codec codec;
    std::string rootDir;    //要同步的文件夹
    //存放此用户对应的TcpConnection，这样可以方便查看此用户连接的次数，以决定此conn的类型
    //在此用户掉线时，也能方便关掉所有的conn
    typedef std::vector<muduo::net::TcpConnectionPtr> Con_Vec;
    std::unordered_map<std::string,Con_Vec>   userMaps;
    muduo::MutexLock userMaps_mutex;
    //正在发送的文件和conn数组
    std::unordered_map<std::string,Con_Vec> sendfileMaps;
    //文件名和md5的map
    std::unordered_map<std::string,std::string> md5Maps;
    muduo::MutexLock md5Maps_mutex;

    typedef std::function<void(muduo::net::TcpConnectionPtr)> SendTask;
    //发送文件的任务列表pair第一个元素为文件名
    typedef std::list<std::pair<std::string,SendTask>> TaskList;
    //每个用户一个任务列表
    std::unordered_map<std::string,TaskList> sendListMaps;
    muduo::MutexLock sendListMaps_mutex;

    //连接到来的处理函数
    void onConnection(const muduo::net::TcpConnectionPtr &conn);
    //接收原始包的处理函数
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                  muduo::net::Buffer *buf,muduo::Timestamp receiveTime);
    //连接断开的处理函数
    void onShutDown(const muduo::net::TcpConnectionPtr &conn);

    //接收到相应protobuf的处理函数
    void onSignIn(const muduo::net::TcpConnectionPtr &conn, const SignInPtr &message);
    void onInit(const muduo::net::TcpConnectionPtr &conn, const InitPtr &message);
    void onSyncInfo(const muduo::net::TcpConnectionPtr &conn, const SyncInfoPtr &message);
    void onSendFile(muduo::net::TcpConnectionPtr &conn,MessagePtr message);
    void onFileInfo(const muduo::net::TcpConnectionPtr &conn, const FileInfoPtr &message);

    //找出客户端不存在的文件并发送给客户端
    void recvFile(const muduo::net::TcpConnectionPtr &conn,Info_ConnPtr &info_ptr);
    void sendfileWithproto(const muduo::net::TcpConnectionPtr &conn, std::string &localname);
    void onWriteComplete(const muduo::net::TcpConnectionPtr &conn);
    muduo::net::TcpConnectionPtr getIdleConn(std::string ip); //获取该客户端的空闲文件传输连接
    void doNextSend(const muduo::net::TcpConnectionPtr &conn);//执行下一个发送任务
    void syncToClient(const muduo::net::TcpConnectionPtr &conn, std::string dir, std::vector<std::string> &files);
    void initMd5(std::string dir);
};

#endif // SYNCSERVER_H
