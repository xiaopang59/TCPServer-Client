#include "sckutil.h"

/* read函数得调用方法
int ret;
ret = read_timeout(fd, 5);
if (ret == 0) {
    read(fd, ...);
}
else if(ret == -1 && errno == ETIMEDOUT) {
    timeout...
}
else {
    ERR_EXIT("read_timeout");
}
*/

static int  activate_nonblock(int fd);
static int deactive_nonblock(int fd);

/**
 * read_timeout - 读超时检测函数，不含读操作
 * @fd：文件描述符
 * @wait_seconds：等待超时秒数，如果为0表示不检测超时
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
int read_timeout(int fd, unsigned int wait_seconds)
{
    int ret = 0;
    if (wait_seconds > 0) {
        fd_set read_fdset;
        struct timeval timeout;

        FD_ZERO(&read_fdset);
        FD_SET(fd, &read_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        //select返回值三态
        //1 若timeout时间到（超时），没有检测到读事件 ret返回=0
        //2 若ret返回<0 && errno == EINTR
        //说明select的过程中被别的信号中断（可中断睡眠原理）
        //2-1 若返回-1，select出错
        //3 若ret返回值>0 表示有read事件发生，返回事件发生的个数

        do {
            ret = select(fd + 1, &read_fdset, NULL, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1)
            ret = 0;
    }

    return ret;
}


/**
 * write_timeout - 写超时检测函数，不含写操作
 * @fd：文件描述符
 * @wait_seconds：等待超时间秒数，如果为0表示不检测超时
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
int write_timeout(int fd, unsigned int wait_seconds)
{
    int ret = 0;
    if (wait_seconds > 0) {
        fd_set write_fdset;
        struct timeval timeout;

        FD_ZERO(&write_fdset);
        FD_SET(fd, &write_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        do {
            ret = select(fd + 1, NULL, &write_fdset, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1)
            ret = 0;
    }

    return ret;

}


/**
 * accept_timeout - 带超时的accept
 * @fd：套接字
 * @addr：输出参数，返回对方地址
 * @wait_seconds：等待超时秒数，如果为0表示正常模式
 * 成功（未超时）返回已连接套接字，超时返回-1 并且errno = ETIMEDOUT
 */
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
    int ret = 0;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    if (wait_seconds > 0) {
        fd_set accept_fdset;
        struct timeval timeout;

        FD_ZERO(&accept_fdset);
        FD_SET(fd, &accept_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        do {
            ret = select(fd + 1, &accept_fdset, NULL, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);


        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
            return ret;
        }
        else if (ret == 1) 
            ret = 0;
    }

    //一旦检测出 有select事件发生，表示对等方完成了三次握手，客户端有新连接建立
    //此时再调用accept将不会堵塞
    if (addr != NULL)
        ret = accept(fd, (struct sockaddr*)addr, &addrLen);     //返回已连接套接字
    else
        ret = accept(fd, NULL, NULL);

    if (ret == -1) {
        ret = errno;
        printf("func accept() err:%d\n", ret);
    }

    return ret;
}


/**
 * activate nonblock - 设置I/O为非阻塞模式
 * @fd：文件描述符
 */
static int activate_nonblock(int fd)
{
    int flags;
    int ret = 0;
    if ( (flags = fcntl(fd, F_GETFL)) == -1) {
        ret = errno;
        printf("func() deactive_nonblock err:%d\n", ret);
        return ret;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        ret = errno;
        printf("func() deactive_nonblock err:%d\n", ret);
        return ret;    
    }

    return ret;
}

/**
 * deactvie nonblock - 设置I/O为非阻塞模式
 * @fd：文件描述符
 */
static int deactive_nonblock(int fd)
{   
    int flags;
    int ret = 0;
    if ( (flags = fcntl(fd, F_GETFL)) == -1) {
        ret = errno;
        printf("func() deactive_nonblock err:%d\n", ret);
        return ret;
    }

    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        ret = errno;
        printf("func() deactive_nonblock err:%d\n", ret);
        return ret;
    }

    return ret;
}

/**
 * connect timeout - connect
 * @fd：套接字
 * @addr：要连接的对方地址
 * @wait_seconds：等待超时秒数，如果为0表示正常模式
 * 成功（未超时）返回0，失败返回-1，超时返回-1 并且errno = ETIMEDOUT
 */
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
    int ret;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    if (wait_seconds > 0)
        activate_nonblock(fd);

    ret = connect(fd, (struct sockaddr*)addr, addrLen);
    if (ret < 0 && errno == EINPROGRESS) {
        fd_set connect_fdset;
        struct timeval timeout;

        FD_ZERO(&connect_fdset);
        FD_SET(fd, &connect_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        do 
        {
            //一旦建立连接，则套接字就可写 所以connect_fdset放在了集合种
            ret = select(fd+1, NULL, &connect_fdset, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT; 
        }   
        else if (ret < 0) {
            return -1;
        }
        else if (ret == 1) {
            /* ret返回为1（表示套接字可写），可能有两种情况，一种是连接建立成功，一种是套接字产生错误 */
            /* 此时错误信息不会保存至errno变量中，因此，需要调用getsockopt来获取 */
            int err;
            socklen_t socklen = sizeof(err);
            int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
            if (sockoptret == -1) {
                return -1;
            }

            if (err == 0) {
                ret = 0;
            }
            else {
                errno = err;
                ret = -1;
            }
        }    
    }

    if (wait_seconds > 0) {
        deactive_nonblock(fd);
    }

    return ret;
}

/**
 * readn - 读取固定字节数
 * @fd：文件描述符
 * @buf：接收缓冲区
 * @count：要读取得字节数
 * 成功返回count，失败返回-1，读到EOF返回<count
 */
ssize_t readn(int fd, void *buf, size_t count)
{
    size_t  nLeft = count;   
    ssize_t nRead;          
    char    *bufp = (char *) buf;   
    
    while (nLeft > 0) {     
        if ( (nRead = read(fd, bufp, nLeft)) < 0) { 
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if(nRead == 0) {
            return count - nLeft;
        }
        
        bufp += nRead;  
        nLeft -= nRead; 
    }

    return count;
}

/**
 * writen - 发送固定字节数
 * @fd：文件描述符
 * @buf：发送缓冲区
 * @count：要读取得字节数
 * 成功返回count，失败返回-1
 */
ssize_t writen(int fd, const void *buf, ssize_t count)
{
    size_t  nLeft = count;
    ssize_t nWritten;       
    char *bufp = (char *) buf;  

    while (nLeft > 0) {     
        if ( (nWritten = write(fd, bufp, nLeft)) < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }        
        else if (nWritten == 0) {
            continue;
        }

        bufp += nWritten;       
        nLeft -= nWritten;
    }

    return count;
}

/**
 * recv_peek - 仅仅查看套接字缓冲区数据，但不移除数据
 * @sockfd：套接字
 * @buf：接收缓冲区
 * @len：长度
 * 成功返回>=0，失败返回-1
 */
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
    while (1) {
        int ret = recv(sockfd, buf, len, MSG_PEEK);
        if (ret == -1 && errno == EINTR)
            continue;
        return ret;
    }
}

/**
 * readline - 按行读取数据
 * @sockfd：套接字
 * @buff：接收缓冲区
 * @maxline：每行最大长度
 * 成功返回>=0，失败返回-1
 */
ssize_t readline(int sockfd, void *buf, size_t maxline)
{
    int     ret;
    int     nRead;
    char    *bufp = buf;   
    int     nLeft = maxline;    
    int     i;

    while (1) {
        ret = recv_peek(sockfd, bufp, nLeft);
        if (ret < 0)    
            return ret;
        else if (ret == 0)
            return ret;

        nRead = ret;
        for (i = 0; i < nRead; i++) {
            if (bufp[i] == '\n') {
                ret = readn(sockfd, bufp, i+1);
                if (ret < 0) {
                    break;
                }
                else if (ret != i + 1) {
                    printf("client close.\n");
                    exit(-1);
                }

                return ret; 
            }
        }

        if (nRead > nLeft) {
            close(sockfd);
            exit(-1);
        }

        nLeft -= nRead;     
        ret = readn(sockfd, bufp, nRead);
        if (ret == -1) {
            break;
        }
        else if (ret != nRead) {
            printf("client close.\n");
            exit(-1);
        }

        bufp += nRead;
    } 

    close(sockfd);
    ERR_EXIT("readn");
}


