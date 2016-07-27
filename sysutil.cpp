#include "sysutil.h"

namespace sysutil{


/**
 * read_timeout - 读超时检测函数，不含读操作
 * @fd: 文件描述符
 * @wait_seconds: 等待超时秒数，如果为0表示不检测超时
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
int read_timeout(int fd, unsigned int wait_seconds)
{
    int ret = 0;
    if (wait_seconds > 0)
    {
        fd_set read_fdset;
        struct timeval timeout;

        FD_ZERO(&read_fdset);
        FD_SET(fd, &read_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do
        {
            ret = select(fd + 1, &read_fdset, NULL, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == 0)
        {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1)
            ret = 0;
    }

    return ret;
}

/**
 * write_timeout - 读超时检测函数，不含写操作
 * @fd: 文件描述符
 * @wait_seconds: 等待超时秒数，如果为0表示不检测超时
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
int write_timeout(int fd, unsigned int wait_seconds)
{
    int ret = 0;
    if (wait_seconds > 0)
    {
        fd_set write_fdset;
        struct timeval timeout;

        FD_ZERO(&write_fdset);
        FD_SET(fd, &write_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do
        {
            ret = select(fd + 1, NULL, NULL, &write_fdset, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == 0)
        {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1)
            ret = 0;
    }

    return ret;
}

/**
 * readn - 读取固定字节数
 * @fd: 文件描述符
 * @buf: 接收缓冲区
 * @count: 要读取的字节数
 * 成功返回count，失败返回-1，读到EOF返回<count
 */
ssize_t readn(int fd, void *buf, size_t count)
{
    size_t nleft = count;
    ssize_t nread;
    char *bufp = (char*)buf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if (nread == 0)
            return count - nleft;

        bufp += nread;
        nleft -= nread;
    }

    return count;
}

/**
 * writen - 发送固定字节数
 * @fd: 文件描述符
 * @buf: 发送缓冲区
 * @count: 要读取的字节数
 * 成功返回count，失败返回-1
 */
ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t nleft = count;
    ssize_t nwritten;
    char *bufp = (char*)buf;

    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if (nwritten == 0)
            continue;

        bufp += nwritten;
        nleft -= nwritten;
    }

    return count;
}
/**
 * @brief filetranse   先发送文件大小(int类型)，然后发送文件
 * @param desSocket    与对方连接的socket
 * @param filename     文件相对路径+文件名
 */
void fileSend(int desSocket,char *filename)
{
    int fd = open(filename,O_RDONLY);
    if(-1 == fd)
        CHEN_LOG(ERROR,"open file error");
    struct stat sbuf;
    int ret = fstat(fd,&sbuf);
    if (!S_ISREG(sbuf.st_mode))
    {//不是一个普通文件
        CHEN_LOG(WARN,"failed to open file");
        return;
    }
    int bytes_to_send = sbuf.st_size; //  文件大小
    CHEN_LOG(DEBUG,"file size:%d\n",bytes_to_send);
    writen(desSocket,&bytes_to_send,sizeof(bytes_to_send)); //先发送文件大小
    //开始传输
    while(bytes_to_send)
    {
        int num_this_time = bytes_to_send > 4096 ? 4096:bytes_to_send;
        ret = sendfile(desSocket,fd,NULL,num_this_time);//不会返回EINTR,ret是发送成功的字节数
        if (-1 == ret)
        {
            //发送失败
            CHEN_LOG(WARN,"send file failed");
            break;
        }
        bytes_to_send -= ret;//更新剩下的字节数
    }
}

/**
 * @brief fileRecv 接收文件
 * @param recvSocket 与对方连接的socket
 * @param filename   文件相对路径+文件名
 * @param size       文件大小
 */
void fileRecv(int recvSocket,char *filename,int size)
{
    char buf[1024];
    // 打开文件
    int fd = open(filename, O_CREAT | O_WRONLY, 0666);
    if (fd == -1)
    {
        CHEN_LOG(ERROR,"Could not create file.");
        return;
    }
    while(size)
    {
        int ret = read(recvSocket, buf, sizeof(buf));
        if (ret == -1)
        {
            if (errno == EINTR)
            {//被信号中断
                continue;
            }
            else
            {//读取失败
                CHEN_LOG(WARN,"failed to recv file");
                break;
            }
        }
        else if(ret == 0 )
            return;
        else if(ret > 0)
            size -= ret;
    }
}

}
