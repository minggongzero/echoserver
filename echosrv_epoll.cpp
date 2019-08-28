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
#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>
#include "myecho.h"

using namespace std;

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

int setnonblock(int sock)
{
	int arg = fcntl(sock, F_GETFL);
	if(arg == -1) ERR_EXIT("fcntl");
	arg |= O_NONBLOCK;
	int ret = fcntl(sock, F_SETFL, arg);
	if(ret == -1) ERR_EXIT("fcntl");
	return 0;
}

int setblock(int sock)
{
	int arg = fcntl(sock, F_GETFL);
	if(arg == -1) ERR_EXIT("fcntl");
	arg &= ~O_NONBLOCK;
	int ret = fcntl(sock, F_SETFL, arg);
	if(ret == -1) ERR_EXIT("fcntl");
	return 0;
}

void echsrv_epoll()
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
    
    struct epoll_event event;
    vector<struct epoll_event> events(16);
    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    event.data.fd = listenfd;
    event.events = EPOLLIN; 
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) < 0) 
	    ERR_EXIT("epoll_ctl");
    int count = 0;    //for test
    while(1)
    {
	int ret = epoll_wait(epollfd, &*(events.begin()), static_cast<int>(events.size()), -1);
	if(ret < 0) ERR_EXIT("epoll_wait");
	if(ret == events.size()) events.resize(ret * 2);
	for(int i=0; i<ret; ++i)
	{
		if(events[i].data.fd == listenfd)
		{
			int conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen);
			if(conn < 0) ERR_EXIT("accept");
        		printf("ip:%s, port:%d\n", inet_ntoa(peeraddr.sin_addr), peeraddr.sin_port);
			setnonblock(conn);
			event.data.fd = conn;
			event.events = EPOLLIN | EPOLLET;
			if(epoll_ctl(epollfd, EPOLL_CTL_ADD, conn, &event) < 0)
				ERR_EXIT("epoll_ctl");
			printf("------%d------\n", ++count);    //for test
		}
		else if(events[i].events & EPOLLIN)
		{
			if(events[i].data.fd < 0) continue;
			char recvbuf[1024] = {0};
		    	int re;
			if((re = readline(events[i].data.fd, recvbuf, sizeof(recvbuf))) < 0)
				ERR_EXIT("readline");
		    	else if(re == 0)
		    	{
				close(events[i].data.fd);
				event = events[i];
		    		epoll_ctl(epollfd, EPOLL_CTL_DEL, event.data.fd, &event);	
				printf("ret = %d", ret);
				printf("client closed\n");
				continue;
		    	}
		    	fputs(recvbuf, stdout);
		    	writen(events[i].data.fd, recvbuf, re);
		}
	}
    }
}
