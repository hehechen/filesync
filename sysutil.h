#ifndef SYSUTIL_H
#define SYSUTIL_H

#include "common.h"
#include "syncserver.h"
#include <muduo/net/TcpConnection.h>
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>
/*系统工具*/
namespace sysutil{
    ssize_t writen(int fd, const void *buf, size_t count);
    //从应用层缓冲区读取文件数据，append到文件后面
    void fileRecvfromBuf(const char *filename, const char *buf, int size);

    //先发送fileInfo信息再发送文件内容
    void sendfileWithproto(const muduo::net::TcpConnectionPtr &conn,Info_ConnPtr &info_ptr,
                           const char* localname,const char *remotename);
    //发送syncInfo信息
    void send_SyncInfo(const muduo::net::TcpConnectionPtr &conn, int id, std::string filename,
                                        std::string newname = "", int removedSize=-1);
    //同步整个文件夹
    void sync_Dir(int socketfd, std::string root, std::string dir);
    //利用qt库获取md5，支持大文件
    std::string getFileMd5(std::string filePath);
    std::string getStringMd5(char *buffer);

    unsigned int adler32(char * data, int len);
    unsigned int adler32_rolling_checksum(unsigned int csum, int len, char c1, char c2);
}
#endif // SYSUTIL_H
