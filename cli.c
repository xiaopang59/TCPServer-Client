#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "commsocket.h"

#define MAX_LEN 1024

int main(void)
{
    int             ret = 0;
    void            *handle = NULL;
    unsigned char   sendBuf[MAX_LEN] = {0};
    unsigned char   recvBuf[MAX_LEN] = {0};
    int             recvLen = 0; 
    int             connfd;

    if (fgets((char*)sendBuf, sizeof(sendBuf), stdin) == NULL)
        ERR_EXIT("fgets");
    
    //客户端环境初始化
    //ret = sockClient_init(&handle, "127.0.0.1", 23456, 15, 5, 5);
    ret = sockClient_init(&handle, 15, 5, 5, 10);

    ret = sockClient_getConnect(handle, "127.0.0.1", 23456, &connfd);

    //客户端发送报文
    ret = sockClient_send(handle, connfd, sendBuf, (int)strlen((char*)sendBuf));
    //printf("%s", sendBuf);
    if (ret == Sock_ErrTimeout){
        //ret = sockClient_send(handle, connfd, sendBuf, (int)strlen((char*)sendBuf));
    }
    else if (ret < 0) {
#if 0
        ret = sockClient_init(&handle, 15, 5, 5, 10);
        ret = sockClient_getConnect(handle, "127.0.0.1", 23456, &connfd);
#endif
    }

    recvLen = sizeof(recvLen);
    //客户端接收报文
    ret = sockClient_recv(handle, connfd, recvBuf, &recvLen);
    printf("%s\n", recvBuf);

    //客户端环境释放
    ret = sockClient_destory(handle);

    exit(0);
}
