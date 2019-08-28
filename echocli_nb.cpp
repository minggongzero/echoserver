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

void echcli_nb()
{
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ERR_EXIT("socket");
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5188);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("connect");
    struct packet sendbuf, recvbuf;
    memset(&sendbuf, 0, sizeof(sendbuf));
    memset(&recvbuf, 0, sizeof(recvbuf));
    while(fgets(sendbuf.buf, sizeof(sendbuf.buf), stdin) != NULL)
    {
        int n = strlen(sendbuf.buf);
        sendbuf.len = htonl(n);
        writen(sock, &sendbuf, 4+n);
        int ret = readn(sock, &recvbuf.len, 4);
        if(ret == -1)
            ERR_EXIT("read");
        else if(ret < 4)
        {
            printf("sever close\n");
            break;
        }
        n = ntohl(recvbuf.len);
        ret = readn(sock, recvbuf.buf, n);
        if(ret == -1)
            ERR_EXIT("read");
        else if(ret < n)
        {
            printf("sever close\n");
            break;
        }
        fputs(recvbuf.buf, stdout);
        memset(&sendbuf, 0, sizeof(sendbuf));
        memset(&recvbuf, 0, sizeof(recvbuf));
    }
    close(sock);
}
