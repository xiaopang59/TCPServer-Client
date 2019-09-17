#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "commsocket.h"

#define MAX_LEN 1024

void handler(int signo)
{
    int mypid = 0;
    while ( ( mypid = waitpid(-1, NULL, 0)) > 0) {
        printf("mypid = %d\n", mypid);
    }
}

int main(void)
{
    int     listenfd;     //定义了监听socket的文件描述符
    int     connfd;
    int     ret;
    pid_t   pid;

    if (signal(SIGCHLD, handler) == SIG_ERR)
        ERR_EXIT("signal");
    
    ret = sockServer_init(23456, &listenfd);
    if (ret != Sock_Ok) {
        printf("func sockServer_init() err: %d\n", ret);
        return ret;
    }

    while (1) {
        ret = sockServer_accept(listenfd, &connfd, 5);
        if (ret == Sock_ErrTimeout) {
            printf("Server func sockServer_accept() time out!\n");
            continue;
        }

        pid = fork();
        if (pid < 0)
            ERR_EXIT("fork");

        if (pid == 0) {
            unsigned char   recvBuf[MAX_LEN] = {0};
            int             datalen = 0;
            
            ret = sockClose(listenfd);
            if (ret != 0) {
                printf("func sockClose() err:%d\n", ret);
                return ret;
            }

            while (1) {
                ret = sockServer_recv(connfd, recvBuf, &datalen, 5);
                if (ret == Sock_ErrTimeout) {
                    printf("Server func sockServer_recv() time out!\n");
                    continue;
                }
                else if (ret < 0) {
                    printf("Server func sockServer_recv() err:%d\n", ret);
                    break;
                }

                ret = sockServer_send(connfd, recvBuf, datalen, 5);
                if (ret == Sock_ErrTimeout) {
                    printf("Server func sockServer_send() time out!\n");
                    continue;
                }
                else if (ret < 0) {
                    printf("Server func sockServer_send() err:%d\n", ret);
                    break;
                }
            }
            ret = sockClose(listenfd);
            if (ret != 0) {
                printf("func sockClose() err:%d\n", ret);
                return ret;
            }
            exit(ret);
        }
        else {
            ret = sockClose(connfd);
            if (ret != 0) {
                printf("func sockClose() err:%d\n", ret);
                return ret;
            }
        }
    }

    exit(0);
}
