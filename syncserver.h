#ifndef SYNCSERVER_H
#define SYNCSERVER_H
#include <iostream>
#include <unordered_map>
#include <string>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
using namespace std;


class SyncServer
{
public:
    SyncServer(muduo::net::EventLoop *loop, int port);
    void start()    {   server_.start();    }
private:
    static const int KHeaderLen = 4;    //包的长度信息为4字节
    //TcpConnectionPtr的context存放着此conn的类型
    enum ConType {  CONTROL=1,DATA_SMALL,DATA_BIG };
    //TcpConnection的context存放的结构体
    struct Info_Conn{
        ConType type;
        string filename;    //让客户端同步的文件名
        int total_size;     //文件的大小
        int remain_size;    //还没接收的大小
    };
    //存放此ip对应的TcpConnection，这样可以方便查看此ip连接的次数，以决定此conn的类型
    //在此ip掉线时，也能方便关掉所有的conn
    typedef vector<muduo::net::TcpConnectionPtr> Con_Vec;
    unordered_map<muduo::string,Con_Vec>   ipMaps;
    muduo::net::InetAddress serverAddr;
    muduo::net::TcpServer server_;
    //连接到来的处理函数
    void onConnection(const muduo::net::TcpConnectionPtr &conn);
    //接收原始包的处理函数
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                  muduo::net::Buffer *buf,muduo::Timestamp receiveTime);
    void onStringMessage(const muduo::net::TcpConnectionPtr&conn,
                   const string& message, muduo::Timestamp);       //解析消息后的处理
};

#endif // SYNCSERVER_H
