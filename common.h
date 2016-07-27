/**
  *@brief：主要的库头文件及一些声明
  */

#ifndef _COMMON_H
#define _COMMON_H

#include <iostream>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>//套接字编程
#include <netinet/in.h>//地址
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <map>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <signal.h>
#include <sys/sendfile.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <sys/epoll.h>

//'\'后面不要加注释
/**
 *FTPD_LOG - 日志宏
 *输出日期，时间，日志级别，源码文件，行号，信息
 */
 //定义一个日志宏
#define DEBUG 0
#define INFO  1
#define WARN  2
#define ERROR 3
#define CRIT  4

static const char* LOG_STR[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "CRIT"
};

#define CHEN_LOG(level, format, ...) do{ \
    char msg[1024];                        \
    char buf[32];                                   \
    sprintf(msg, format, ##__VA_ARGS__);             \
    if (level >= 0) {\
        time_t now = time(NULL);                      \
        strftime(buf, sizeof(buf), "%Y%m%d %H:%M:%S", localtime(&now)); \
        fprintf(stdout, "[%s] [%s] [file:%s] [line:%d] %s\n",buf,LOG_STR[level],__FILE__,__LINE__, msg); \
        fflush (stdout); \
    }\
     if (level >= ERROR) {\
        perror(msg);    \
        exit(-1); \
    } \
} while(0)


#endif
