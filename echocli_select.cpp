#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
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

void echcli_select()
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
    fd_set rfds;
    while(1)
    {
    	FD_ZERO(&rfds);
    	FD_SET(sock, &rfds);
    	FD_SET(0, &rfds);
	//int ret = select(sock + 1, &rfds, NULL, NULL, NULL);
	int ret = select(sock + 1, &rfds, NULL, NULL, NULL);
	if(ret < 0)
		ERR_EXIT("select");
	if(FD_ISSET(0, &rfds))
	{
		char sendbuf[1024] = {0};
    		if(fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
        	    write(sock, sendbuf, strlen(sendbuf));
	}
	else if(FD_ISSET(sock, &rfds))
	{
		char recvbuf[1024] = {0};
        	read(sock, recvbuf, sizeof(recvbuf));
		fputs(recvbuf, stdout);
	}
    }
/*
    char sendbuf[1024] = {0};
    char recvbuf[1024] = {0};
    while(fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {
        write(sock, sendbuf, strlen(sendbuf));
        read(sock, recvbuf, sizeof(recvbuf));
        fputs(recvbuf, stdout);
        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }
*/
    close(sock);
}
