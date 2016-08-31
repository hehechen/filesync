#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "../common.h"

/* 服务器要监听的本地端口 */
#define MYPORT 4000
/* 能够同时接受多少没有 accept 的连接 */
#define BACKLOG 10
int new_fd = -1;// 全局变量 连接套接字 主函数和SIGURG信号处理函数中均会调用
void sig_urg(int signo);

int main()
{
    /* 在 sock_fd 上进行监听，new_fd 接受新的连接 */
    int sockfd;
    /* 自己的地址信息 */
    struct sockaddr_in my_addr;
    /* 连接者的地址信息*/
    struct sockaddr_in their_addr;
    int sin_size;
    int n ;
    char buff[100] ;
    /* 用于存储以前系统缺省的 SIGURL 处理器的变量 */
    sighandler_t old_sig_urg_handle ;
    /* 这里就是我们一直强调的错误检查．如果调用 socket() 出错，则返回 */

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {/* 输出错误提示并退出 */
        perror("socket");
        exit(1);
    }
    /* 主机字节顺序 */
    my_addr.sin_family = AF_INET;
    /* 网络字节顺序，短整型 */
    my_addr.sin_port = htons(MYPORT);
    /* 将运行程序机器的 IP 填充入 s_addr */
    my_addr.sin_addr.s_addr = INADDR_ANY;
    /* 将此结构的其余空间清零 */
    bzero(&(my_addr.sin_zero), 8);
    /* 这里是我们一直强调的错误检查！！ */
    if (bind(sockfd, (struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1)
    {
        /* 如果调用 bind()失败，则给出错误提示，退出 */
        perror("bind");
        exit(1);
    }
    /* 这里是我们一直强调的错误检查！！ */
    if (listen(sockfd, BACKLOG) == -1)
    {
        /* 如果调用 listen 失败，则给出错误提示，退出 */
        perror("listen");
        exit(1);
    }


    /* 设置 SIGURG 的处理函数　sig_urg */
    old_sig_urg_handle = signal(SIGURG, sig_urg);
    /* 将我们的进程创建为套接口的拥有者 */ //wrong；非监听套接字
    /*if(fcntl(sockfd, F_SETOWN, getpid())==-1)
{
perror("fcntl");
exit(1);
}*/


    while(1)
    {
        /* 这里是主 accept()循环 */
        sin_size = sizeof(struct sockaddr_in);

        /* 这里是我们一直强调的错误检查！！ */
        if ((new_fd = accept(sockfd, (struct sockaddr *)NULL, NULL)) == -1)
        {
            /* 如果调用 accept()出现错误，则给出错误提示，进入下一个循环 */
            perror("accept");
            continue;
        }
        //printf("in main, new_fd = %d\n",new_fd);
        fcntl(new_fd,F_SETOWN,getpid());//将我们的进程设置为（连接套接字）的拥有者

        /* 服务器给出出现连接的信息 */
        printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
        /* 这里将建立一个子进程来和刚刚建立的套接字进行通讯 */
        //if (!fork())
        if (1)
        {
            while(1)
            {
                if((n=recv(new_fd,buff,sizeof(buff)-1,0)) == 0)
                {
                    printf("received EOF\n");
                    break;
                }
                buff[n] ='\0';
                printf("Recv %d bytes: %s\n", n, buff);
            }
        }
        /* 关闭 new_fd 代表的这个套接字连接 */
        close(new_fd);
    }
    /* 等待所有的子进程都退出 */
    while(waitpid(-1,NULL,WNOHANG) > 0);
    /* 恢复系统以前对 SIGURG 的处理器 */
    signal(SIGURG, old_sig_urg_handle);
}

void sig_urg(int signo)
{
    int n;
    char buff[100];
    printf("SIGURG received\n");
    //printf("in sig_urg(),new_fd = %d\n",new_fd);
    //while((n = recv(new_fd,buff,sizeof(buff)-1,MSG_OOB)) == -1);
    n = recv(new_fd,buff,sizeof(buff)-1,MSG_OOB);
    if(n>0)
    {
        buff[n]='\0';
        printf("recv %d OOB byte: %s\n",n,buff);
    }
    else
    {
        perror("recv");
    }

}
