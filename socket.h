#ifndef SOCKET_H
#define SOCKET_H
#include <iostream>
#include "common.h"
using namespace std;

class TcpSocket
{
public:
    TcpSocket();
    ~TcpSocket();
    void bind(int port);
    void listen();
    int accept();
    void shutdown();
private:
    struct sockaddr_in servaddr;
    int socket_fd;
    string ip;  //对方的ip和端口
    int port;
};

#endif // SOCKET_H
