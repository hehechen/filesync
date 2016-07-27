#include "socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

TcpSocket::TcpSocket()
{   
    //初始化Socket
    if( (socket_fd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1 )
        CHEN_LOG(ERROR,"create socket error");
}

TcpSocket::~TcpSocket()
{
    ::close(socket_fd);
}

void TcpSocket::bind(int port)
{
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。
    servaddr.sin_port = htons(port);//设置的端口为DEFAULT_PORT
    //将本地地址绑定到所创建的套接字上
    if(::bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
       CHEN_LOG(ERROR,"bind error");
    }
}

void TcpSocket::listen()
{
    //开始监听是否有客户端连接
    if(::listen(socket_fd, SOMAXCONN) == -1)
    {
       CHEN_LOG(ERROR,"listen error");
    }
}

int TcpSocket::accept()
{
    int connect_fd;
    if( (connect_fd = ::accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1)
    {
        CHEN_LOG(ERROR,"accept error");
    }
    return connect_fd;
}
