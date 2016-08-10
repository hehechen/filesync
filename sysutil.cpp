#include "sysutil.h"
#include <iostream>
#include <string>
#include "codec.h"
#include "protobuf/filesync.pb.h"

using namespace std;
namespace sysutil{

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
//从应用层缓冲区读取文件数据，append到文件后面
void fileRecvfromBuf(const char *filename,const char *buf,int size)
{
    int fd = open(filename, O_CREAT | O_WRONLY, 0666);
    if (fd == -1)
    {
        CHEN_LOG(ERROR,"Could not create file %s",filename);
        return;
    }
    lseek(fd,0,SEEK_END);
    if(write(fd,buf,size) < 0)
        CHEN_LOG(ERROR,"write file %s to %d error",filename,fd);
    close(fd);
}

/**
 * @brief send_SyncInfo  发送SyncInfo信息，线程安全
 * @param conn
 * @param id    0是创建文件夹，1是create文件，2是modify,3是删除，4是重命名
 * @param filename
 * @param newname       新文件名，发送重命名信息时才用
 * @param removedSize   删除正在发送的文件时，已发送的大小
 */
void send_SyncInfo(const muduo::net::TcpConnectionPtr &conn, int id,
                   string filename, string newname,int removedSize)
{
    filesync::SyncInfo msg;
    msg.set_id(id);
    msg.set_filename(filename);
    msg.set_size(removedSize);
    if(4 == id) //重命名
        msg.set_newfilename(newname);
    string send = Codec::enCode(msg);
    conn->send(send);
}

/**
 * @brief getFileMd5    利用qt库获取md5，支持大文件
 * @param filePath      文件名
 * @return
 */
string getFileMd5(string filePath)
{
    QFile localFile(QString::fromStdString(filePath));

    if (!localFile.open(QFile::ReadOnly))
    {
        qDebug() << "file open error.";
        return 0;
    }

    QCryptographicHash ch(QCryptographicHash::Md5);

    quint64 totalBytes = 0;
    quint64 bytesWritten = 0;
    quint64 bytesToWrite = 0;
    quint64 loadSize = 1024 * 4;
    QByteArray buf;

    totalBytes = localFile.size();
    bytesToWrite = totalBytes;

    while (1)
    {
        if(bytesToWrite > 0)
        {
            buf = localFile.read(qMin(bytesToWrite, loadSize));
            ch.addData(buf);
            bytesWritten += buf.length();
            bytesToWrite -= buf.length();
            buf.resize(0);
        }
        else
        {
            break;
        }

        if(bytesWritten == totalBytes)
        {
            break;
        }
    }

    localFile.close();
    QString md5 = ch.result().toHex();
    return md5.toStdString();
}


}
