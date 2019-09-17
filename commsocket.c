#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include "commsocket.h"
#include "sckutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct _SockHandle
{
    int connfd[100];
    int connectCount;
    int sockfd;
    //int connfd;
    int connectTime;
    int sendTime;
    int recvTime;
}SockHandle;

//客户端环境初始化
int sockClient_init(void **handle, int connectTime, int sendTime, int recvTime, int connectCount)
{
    int                 ret = Sock_Ok;
    SockHandle          *tmp;

    if (handle == NULL || connectTime < 0 || sendTime < 0 || recvTime < 0){
        ret = Sock_ErrParam;
        printf("func sockiClient_init() err: %d, cheek (handle == NULL || conectTime < 0 || sendTime < 0 || recvTime < 0)\n", ret);
        return ret;
    }

    tmp = malloc(sizeof(SockHandle));
    if (tmp == NULL) {
        ret = Sock_ErrParam;
        printf("func sockiClient_init() err: %d, malloc SockHandle\n", ret);
        return ret;
    } 

    tmp->connectTime = connectTime;
    tmp->sendTime = sendTime;
    tmp->recvTime = recvTime;
    tmp->connectCount = connectCount;

#if 0
    int i = 0; 
    int sockfd;
    for (; i < connectCount; i++){
        //链表的顺序
        //初始化socket
        if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            ret = errno;
            printf("func sockiClient_init() err: %d, socket\n", ret);
            return ret;
        }
        tmp->sockfd = sockfd;
    }
#endif

    *handle = tmp;
    return 0;
}

int sockClient_getConnect(void *handle, char *ip, int port, int *connfd)
{
    int                 ret = Sock_Ok;
    struct sockaddr_in  srvaddr;
    int                 sockfd;
    SockHandle          *tmp = NULL;

    if (ip == NULL || port < 0 || port > 65535 || connfd == NULL){
        ret = Sock_ErrParam;
        printf("func sockClient_getConnect() err: %d, cheek (ip == NULL || ip < 0 || ip > 65535 || connfd == NULL) \n", ret);
        return ret;
    }
    
    tmp = (SockHandle*)handle;
    
    //初始化socket
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ret = errno;
        printf("func sockiClient_init() err: %d, socket\n", ret);
        return ret;
    }
 
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(port);
    srvaddr.sin_addr.s_addr = inet_addr(ip);

#if 0
    if ( (myconnfd = connect(sockfd, (struct sockaddr*) &srvaddr, sizeof(srvaddr))) < 0) {
        ret = errno;
        printf("func sockiClient_init() err: %d, connect\n", ret);
        return ret;
    }
#endif

    ret = connect_timeout(sockfd, &srvaddr, tmp->connectTime);
    if (ret == -1 && errno == ETIMEDOUT) {
        ret = Sock_ErrTimeout;
        return ret;
    }
    else if (ret == -1) {
        ret = errno;
        printf("func sockClient_getConnect() err: %d connect_timeout\n", ret);
        return ret;
    }

    *connfd = sockfd;
    return ret;
}

// 发送报文函数
static int sock_send(int connfd, int sendTime, unsigned char *data, int datalen)
{
    int ret = Sock_Ok;

    ret = write_timeout(connfd, sendTime);
    if (ret == 0 ){
        int writed = 0;
        unsigned char *netdata = malloc(datalen + 4);
        if (netdata == NULL) {
            ret = Sock_ErrMalloc;
            printf("func sockClient_send() malloc err: %d\n", ret);
            return ret;
        }

        int netlen = htonl(datalen);
        memcpy(netdata, &netlen, 4);
        memcpy(netdata+4, data, datalen);

#if 0
        int i = 0;
        for (; i < datalen + 4; i++)
            printf("%d ", netdata[i]);
        printf("\n");
#endif

        if ( (writed = writen(connfd, netdata, datalen+4)) != datalen + 4) {
            if (netdata != NULL) {
                printf("func writen() err:%d\n", writed);
                free(netdata);
                netdata = NULL;
            }
            return writed;
        }
    }
    else if (ret == -1 && errno == ETIMEDOUT) { //返回超时
        ret = ETIMEDOUT;
        printf("func Sock_ErrMalloc() write_timeout err:%d\n", ret);
        return ret;
    }

    return ret;
}

//客户端发送报文
int sockClient_send(void *handle, int connfd, unsigned char *data, int datalen)
{
    int             ret = Sock_Ok;
    SockHandle      *tmp = NULL;

    if (handle == NULL || data == NULL || datalen == 0 || connfd < 0) {
        ret = Sock_ErrParam;
        printf("func sockiClient_init() err: %d, (handle == NULL || data == NULL || datalen == 0 || connfd < 0)\n", ret);
        return ret;
    }

    tmp = (SockHandle*)(handle);
    return sock_send(connfd, tmp->sendTime, data, datalen);
}


//接收报文函数
static int sock_recv(int connfd, int recvTime, unsigned char *out, int *outlen)
{
    int             ret = Sock_Ok;
    int             dataLen;
    int             n;
    int             nRead;

    ret = read_timeout(connfd, recvTime);
    if (ret == 0 ){
        if ( (nRead = readn(connfd, &dataLen, 4)) != 4) { //该包头包含4个字节
            if (nRead == -1 && errno == ETIMEDOUT) {
                ret = Sock_ErrTimeout;
                printf("func sockClient_recv timeout err: %d\n", ret);
                return ret;
            }
            else {
                printf("func readn() err peer closed: %d\n", nRead);
                return nRead;
            }
        }

        n = htonl(dataLen);
        if ( (nRead = readn(connfd, out, n)) != n) { //根据长度读数据
            if (nRead == -1 && errno == ETIMEDOUT) {
                ret = Sock_ErrTimeout;
                printf("func sockClient_recv timeout err: %d\n", ret);
                return ret;
            }
            else {
                printf("func readn() err peer closed: %d\n", nRead);
                return nRead;
            }
        }
    }
    else if (ret == -1 && errno == ETIMEDOUT) {  //返回超时
        ret = ETIMEDOUT;
        printf("func sockClient_recv timeout err:%d\n", ret);
        return ret;
    }

    *outlen = n;

    return ret;
}

//客户端接收报文
int sockClient_recv(void *handle, int connfd, unsigned char *out, int *outlen)
{
    int             ret = Sock_Ok;
    SockHandle      *tmp = NULL;

    if (handle == NULL || out == NULL || outlen == NULL || connfd < 0) {
        ret = Sock_ErrParam;
        printf("func sockiClient_init() err: %d, (handle == NULL || out == NULL || outlen == NULL || connfd < 0)\n", ret);
        return ret;
    }
    
    tmp = (SockHandle*)handle;
    return sock_recv(connfd, tmp->recvTime, out, outlen);
}

int sockClient_close(int connfd)
{
    int         ret = Sock_Ok;

    if (connfd < 0) {
        ret = Sock_ErrParam;    
        printf("func sockiClient_close() err: %d, (connfd < 0)\n", ret);
        return ret;
    }

    ret = close(connfd);
    return ret;
}

//客户端环境释放
int sockClient_destory(void *handle)
{
    if (handle != NULL) {
        free(handle);
        handle = NULL;
    }
    return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////

//函数声明
//服务器端环境初始化
int sockServer_init(int port, int *listenfd)
{
    struct sockaddr_in  srvaddr;
    int                 ret = Sock_Ok;
    int                 on = 1;
    int                 sockfd;

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(port);
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ret = errno;
        printf("func socket() err:%d", ret);
        return ret;
    }

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret < 0) {
        ret = errno;
        printf("func setsockopt() err:%d\n", ret);
        return ret;
    }

    ret = bind(sockfd, (struct sockaddr*) &srvaddr, sizeof(srvaddr));
    if (ret < 0) {
        ret = errno;
        printf("func bind() err:%d\n", ret);
        return ret;
    }
    
    ret = listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        ret = errno;
        printf("func listen err:%d\n", ret);
        return ret;
    }

    *listenfd = sockfd;
    return ret;
}

int sockServer_accept(int listenfd, int *connfd, int acceptTime)
{
    int ret = Sock_Ok;

    ret = accept_timeout(listenfd, NULL, acceptTime);
    if (ret < 0 && errno != ETIMEDOUT) {
        ret = errno;
        printf("func accept_timeout() err:%d\n", ret);
        return ret;
    }
    else if (ret == -1 && errno == ETIMEDOUT) {
        //ret==0 并且 errno==ETIMEDOUT 表示accept连接超时
        ret = Sock_ErrTimeout;
        printf("func accept_timeout() time out.\n");
        return ret;
    }

    *connfd = ret;
    return ret;
}

//服务器端发送报文
int sockServer_send(int connfd, unsigned char *data, int datalen, int sendTime)
{ 
    int             ret = Sock_Ok;

    if (data == NULL || datalen == 0 || connfd < 0) {
        ret = Sock_ErrParam;
        printf("func sockiClient_init() err: %d, (data == NULL || datalen == 0 || connfd < 0)\n", ret);
        return ret;
    }
   
    return sock_send(connfd, sendTime, data, datalen);
}

//服务器端接收报文
int sockServer_recv(int connfd, unsigned char *out, int *outlen, int recvTime)
{
    int             ret = Sock_Ok;
    
    if (out == NULL || outlen == NULL || connfd < 0) {
        ret = Sock_ErrParam;
        printf("func sockiClient_init() err: %d, (out == NULL || outlen == NULL || connfd < 0)\n", ret);
        return ret;
    }
    
    return sock_recv(connfd, recvTime, out, outlen);
}

int sockClose(int connfd)
{
    int         ret = Sock_Ok;

    if (connfd < 0) {
        ret = Sock_ErrParam;
        printf("func sockiClient_close() err: %d, (connfd < 0)\n", ret);
        return ret;
    }

    ret = close(connfd);
    if (ret == -1) 
        ret = errno;

    return ret;
}


