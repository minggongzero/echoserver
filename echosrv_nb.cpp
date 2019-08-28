#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <errno.h>
#include <cstdio>
#include <cstring>
#include "myecho.h"

#define ERR_EXIT(m) \
            do \
            { \
                perror(m); \
                exit(EXIT_FAILURE); \
            }while(0)

struct packet
{
    int len;
    char buf[1024];
};

ssize_t readn(int fd, void *buf, size_t count)
{
    char *bufp = (char*)buf;
    int nleft = count, ret;
    while(nleft > 0)
    {
        if((ret = read(fd, bufp, nleft)) < 0)
        {
            if(errno == EINTR)
                continue;
            return -1;
        }
        else if(!ret) break;
        bufp += ret;
        nleft -= ret;
    }
    return count - nleft;
}

ssize_t writen(int fd, const void *buf, size_t count)
{
    char *bufp = (char*)buf;
    int nleft = count, ret;
    while(nleft > 0)
    {
        if((ret = write(fd, bufp, nleft)) < 0)
        {
            if(errno == EINTR)
                continue;
            return -1;
        }
        else if(!ret) continue;
        bufp += ret;
        nleft -= ret;
    }
    return count - nleft;
}

void echsrv_nb()
{
    int listenfd;
    if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ERR_EXIT("socket");
    int in = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &in, sizeof(in)) < 0)
        ERR_EXIT("reuseaddr");
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5188);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind");
    if(listen(listenfd, SOMAXCONN) < 0)
        ERR_EXIT("listen");
    struct sockaddr_in peeraddr;
    socklen_t peerlen = sizeof(peeraddr);
    int conn;
    pid_t pid;
    while(1)
    {
        if((conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen)) < 0)
        ERR_EXIT("accept");
        printf("ip:%s, port:%d\n", inet_ntoa(peeraddr.sin_addr), peeraddr.sin_port);
        pid = fork();
        if(pid == -1)
            ERR_EXIT("fork");
        else if(pid == 0)
        {
            close(listenfd);
            struct packet apacket;
            while(1)
            {
                memset(&apacket, 0, sizeof(apacket));
                int ret = readn(conn, &apacket.len, 4);
                if(ret == -1)
                {
                    ERR_EXIT("read");
                }
                else if(ret < 4)
                {
                    printf("client close\n");
                    break;
                }
                int n = ntohl(apacket.len);
                ret = readn(conn, apacket.buf, n);
                if(ret == -1)
                {
                    ERR_EXIT("read");
                }
                else if(ret < n)
                {
                    printf("client close\n");
                    break;
                }
                fputs(apacket.buf, stdout);
                writen(conn, &apacket, n+4);
            }
            close(conn);
            exit(EXIT_SUCCESS);
        }
        else
            close(conn);
    }
}
