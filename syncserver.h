#ifndef SYNCSERVER_H
#define SYNCSERVER_H
#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <queue>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include "codec.h"
#include "protobuf/filesync.pb.h"

//TcpConnectionPtr的context存放着此conn的类型
enum ConType {  CONTROL=1,DATA };
//TcpConnection的context存放的结构体
struct Info_Conn{
    ConType type;
    bool isIdle = true;    //是否可发送
    bool isRecving = false; //是否正在接收文件
    int totalSize = 0;
    int remainSize = 0;
    std::string filename;   //本地文件名包含路径
    FILE *fp;     //要发送的文件的fp
};
typedef std::shared_ptr<Info_Conn> Info_ConnPtr;

//相应protobuf消息的回调函数
typedef std::shared_ptr<filesync::Init> InitPtr;
typedef std::shared_ptr<filesync::SyncInfo> SyncInfoPtr;
typedef std::shared_ptr<filesync::FileInfo> FileInfoPtr;

class SyncServer
{
public:
    SyncServer(const char *root,muduo::net::EventLoop *loop, int port);
    void start()    {   server_.start();    }
private:
    static const int KHeaderLen = 4;    //包的长度信息为4字节

    std::string rootDir;    //要同步的文件夹
    muduo::net::InetAddress serverAddr;
    muduo::net::TcpServer server_;
    Codec codec;
    //存放此ip对应的TcpConnection，这样可以方便查看此ip连接的次数，以决定此conn的类型
    //在此ip掉线时，也能方便关掉所有的conn
    typedef std::vector<muduo::net::TcpConnectionPtr> Con_Vec;
    std::unordered_map<muduo::string,Con_Vec>   ipMaps;

    typedef std::function<void(muduo::net::TcpConnectionPtr)> SendTask;
    typedef std::queue<SendTask> TaskQueue; //发送文件的任务队列
    std::unordered_map<muduo::string,TaskQueue> queueMaps;  //每个ip一个任务队列

    //连接到来的处理函数
    void onConnection(const muduo::net::TcpConnectionPtr &conn);
    //接收原始包的处理函数
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                  muduo::net::Buffer *buf,muduo::Timestamp receiveTime);

    //接收到相应protobuf的处理函数
    void onInit(muduo::net::TcpConnectionPtr &conn,InitPtr message);
    void onSyncInfo(const muduo::net::TcpConnectionPtr &conn, const SyncInfoPtr &message);
    void onSendFile(muduo::net::TcpConnectionPtr &conn,MessagePtr message);
    void onFileInfo(const muduo::net::TcpConnectionPtr &conn, const FileInfoPtr &message);

    //找出客户端不存在的文件并发送给客户端
    bool syncToClient(muduo::net::TcpConnectionPtr &conn);
    void recvFile(const muduo::net::TcpConnectionPtr &conn,Info_ConnPtr &info_ptr);
    void sendfileWithproto(const muduo::net::TcpConnectionPtr &conn, std::string &localname);
    void onWriteComplete(const muduo::net::TcpConnectionPtr &conn);
    muduo::net::TcpConnectionPtr getIdleConn(muduo::string ip); //获取该客户端的空闲文件传输连接
    void doNextSend(const muduo::net::TcpConnectionPtr &conn);//执行下一个发送任务
};

#endif // SYNCSERVER_H
