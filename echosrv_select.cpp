#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <errno.h>
#include <cstdio>
#include <cstring>
#include <signal.h>
#include <sys/select.h>
#include "myecho.h"

#define ERR_EXIT(m) \
            do \
            { \
                perror(m); \
                exit(EXIT_FAILURE); \
            }while(0)

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

ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
	ssize_t ret;
	while(1)
	{
		ret = recv(sockfd, buf, len, MSG_PEEK);
		if(ret == -1) return -1;
		else if(ret == 0 && errno == EINTR) continue;
		return ret;
	}
}

ssize_t readline(int sockfd, void *buf, int maxsize)
{

	int ret, nleft = maxsize;
	char *bufp = (char*)buf;
	while(1)
	{
		
		if((ret = recv_peek(sockfd, bufp, nleft)) < 0)
			ERR_EXIT("recv_peek");
		else if(ret == 0) return maxsize - nleft;
		int i, nread;
		for(i = 0; i < ret; ++i)
		{
			if(bufp[i] == '\n')
			{
				nread = readn(sockfd, bufp, i+1);
				if(nread < 0) return -1;
				return maxsize - nleft + nread;
			}
		}
		if(i == ret)
		{
			nread = readn(sockfd, bufp, ret);
			if(nread != ret) return -1;
			nleft -= nread;
			bufp += nread;
			continue;
		}
	}
	return -1;
}

void handler(int sig)
{
	while( waitpid(-1, NULL, WNOHANG) > 0)
		;
}

void echsrv_select()
{
    signal(SIGCHLD, handler);
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
    fd_set rfds, allset;
    int maxfd = listenfd;
    int client[FD_SETSIZE];
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    int i;
    client[0] = listenfd;
    for(i = 1; i < FD_SETSIZE; ++i) client[i] = -1;
    int count = 0;    //for test
    while(1)
    {
	rfds = allset;
	int ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
	if(ret < 0) ERR_EXIT("select");
	if(FD_ISSET(listenfd, &rfds))
	{
		int conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen);
		if(conn < 0) ERR_EXIT("accept");
        	printf("ip:%s, port:%d\n", inet_ntoa(peeraddr.sin_addr), peeraddr.sin_port);
		for(i = 0; i < FD_SETSIZE; ++i)
		{
			if(client[i] < 0) 
			{
				client[i] = conn;
				FD_SET(conn, &allset);
				if(maxfd < conn) maxfd = conn;
				printf("------%d------\n", ++count);    //for test
				break;
			}
		}
		if(i == FD_SETSIZE) 
			printf("too many clients\n");
		if(--ret <= 0) continue;
	}
	for(i = 1; i < FD_SETSIZE; ++i)
	{
	    if(FD_ISSET(client[i], &rfds))
	    {
		    int re;
		    char recvmsg[1024] = {0};
		    if((re = readline(client[i], recvmsg, sizeof(recvmsg))) < 0)
			ERR_EXIT("readline");
		    else if(re == 0)
		    {
			FD_CLR(client[i], &allset);
			close(client[i]);
			client[i] = -1;
		    	printf("client closed\n");
			continue;
		    }
		    recvmsg[re] = '\0';
		    fputs(recvmsg, stdout);
		    writen(client[i], recvmsg, re);
		    if(--ret <= 0) break; 
	    }
	}
    }
}
