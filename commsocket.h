#ifndef COMM_SOCKET_H__
#define COMM_SOCKET_H__

//错误码定义
#define Sock_Ok             0
#define Sock_BaseErr        3000

#define Sock_ErrParam       (Sock_BaseErr + 1)
#define Sock_ErrTimeout     (Sock_BaseErr + 2)    
#define Sock_ErrPerrClosed  (Sock_BaseErr + 3)
#define Sock_ErrMalloc      (Sock_BaseErr + 4)

#define ERR_EXIT(err_str) \
    do \
    { \
        perror(err_str); \
        exit(EXIT_FAILURE);\
    } while(0)

#ifdef __cplusplus
extern 'C'
{
#endif

//函数声明
//客户端环境初始化
int sockClient_init(void **handle, int conectTime, int sendTime, int recvTime, int connectCount);
int sockClient_getConnect(void *handle, char *ip, int port, int *connfd);
int sockClient_close(int connfd);

//客户端发送报文
int sockClient_send(void *handle, int connfd, unsigned char *data, int datalen);

//客户端接收报文
int sockClient_recv(void *handle, int connfd, unsigned char *out, int *outlen);

//客户端环境释放
int sockClient_destory(void *handle);



//函数声明
//服务器端环境初始化
int sockServer_init(int port, int *listenfd);
int sockServer_accept(int listenfd, int *connfd, int acceptTime);

//服务器端发送报文
int sockServer_send(int connfd, unsigned char *data, int datalen, int sendTime);

//服务器端接收报文
int sockServer_recv(int connfd, unsigned char *out, int *outlen, int recvTime);


int sockClose(int connfd);

#ifdef __cplusplus
}
#endif

#endif
