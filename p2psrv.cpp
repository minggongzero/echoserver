#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
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

void handler(int sig)
{
    printf("recv a single=%d", sig);
    exit(EXIT_SUCCESS);
}
void p2psrv()
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
    if((conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen)) < 0)
        ERR_EXIT("accept");
    printf("ip:%s, port:%d\n", inet_ntoa(peeraddr.sin_addr), peeraddr.sin_port);
    pid = fork();
    if(pid == -1)
        ERR_EXIT("fork");
    else if(pid == 0)
    {
        signal(SIGUSR1, handler);
        char sendbuf[1024];
        memset(sendbuf, 0, sizeof(sendbuf));
        while(fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
        {
            int ret = write(conn, sendbuf, strlen(sendbuf));
            if(ret < 0) ERR_EXIT("write");
            memset(sendbuf, 0, sizeof(sendbuf));
        }
        printf("child close\n");
        //exit(EXIT_FAILURE);
    }
    else
    {
        char recvbuf[1024];
        while(1)
        {
            memset(recvbuf, 0, sizeof(recvbuf));
            int ret = read(conn, recvbuf, sizeof(recvbuf));
            if(ret == 0)
            {
                printf("peer close\n");
                break;
            }
            else if(ret < 0) ERR_EXIT("read");
            fputs(recvbuf, stdout);
        }
        kill(pid, SIGUSR1);
        printf("parent close\n");
    }
    close(listenfd);
    close(conn);
    exit(EXIT_SUCCESS);
}
