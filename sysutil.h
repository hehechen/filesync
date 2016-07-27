#ifndef SYSUTIL_H
#define SYSUTIL_H

#include "common.h"

/*系统工具*/
namespace sysutil{
    int read_timeout(int fd,unsigned int wait_seconds);
    int write_timeout(int fd, unsigned int wait_seconds);

    ssize_t readn(int fd, void *buf, size_t count);
    ssize_t writen(int fd, const void *buf, size_t count);
    ssize_t recv_peek(int sockfd, void *buf, size_t len);
    ssize_t readline(int sockfd, void *buf, size_t maxline);

    void fileSend(int desSocket,char *filename);
    void fileRecv(int recvSocket, char *filename, int size);
}
#endif // SYSUTIL_H
