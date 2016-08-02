#ifndef SYNCSERVER_H
#define SYNCSERVER_H
#include <iostream>
#include <unordered_map>
#include <string>
#include "codec.h"
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include "protobuf/filesync.pb.h"

//相应protobuf消息的回调函数
typedef std::shared_ptr<filesync::syncInfo> syncInfoPtr;
typedef std::shared_ptr<filesync::fileInfo> fileInfoPtr;

class SyncServer
{
public:
    SyncServer(muduo::net::EventLoop *loop, int port);
    void start()    {   server_.start();    }
private:
    static const int KHeaderLen = 4;    //包的长度信息为4字节
    //TcpConnectionPtr的context存放着此conn的类型
    enum ConType {  CONTROL=1,DATA };
    //TcpConnection的context存放的结构体
    struct Info_Conn{
        ConType type;
        bool isIdle = true;    //是否可发送
        bool isRecving = false; //是否正在接收文件
        int totalSize = 0;
        int remainSize = 0;
        std::string filename;
    };
    typedef std::shared_ptr<Info_Conn> Info_ConnPtr;
    //存放此ip对应的TcpConnection，这样可以方便查看此ip连接的次数，以决定此conn的类型
    //在此ip掉线时，也能方便关掉所有的conn
    typedef std::vector<muduo::net::TcpConnectionPtr> Con_Vec;
    std::unordered_map<muduo::string,Con_Vec>   ipMaps;

    Codec codec;
    muduo::net::InetAddress serverAddr;
    muduo::net::TcpServer server_;

    //连接到来的处理函数
    void onConnection(const muduo::net::TcpConnectionPtr &conn);
    //接收原始包的处理函数
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                  muduo::net::Buffer *buf,muduo::Timestamp receiveTime);

    //接收到相应protobuf的处理函数
    void onSyncInfo(const muduo::net::TcpConnectionPtr &conn, const syncInfoPtr &message);
    void onSendFile(muduo::net::TcpConnectionPtr &conn,MessagePtr message);
    void onFileInfo(const muduo::net::TcpConnectionPtr &conn, const fileInfoPtr &message);

    void recvFile(Info_ConnPtr &info_ptr, muduo::net::Buffer *inputBuffer);
};

#endif // SYNCSERVER_H
