#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstdlib>
#include <errno.h>
#include <cstdio>
#include <cstring>
#include "myecho.h"

#define MAXMSG 4096

#define ERR_EXIT(m) \
            do \
            { \
                perror(m); \
                exit(EXIT_FAILURE); \
            }while(0)
/*
struct msgbuf {
        long mtype;        message type, must be > 0 
	char mtext[MAXMSG];     message data 
};
*/

void echcli_msq()
{
	int pid = getpid();
	int msqid = msgget(1234, 0666);
	if(msqid < 0) ERR_EXIT("msgget");
	struct msgbuf msb;
	memset(&msb, 0, sizeof(struct msgbuf));
        *((int*)msb.mtext) = pid;
	while(fgets(msb.mtext+4, MAXMSG, stdin) != NULL)
	{
		msb.mtype = 1;
		if(msgsnd(msqid, &msb, 4+strlen(msb.mtext+4), 0) < 0)
			ERR_EXIT("msgsnd");
		memset(msb.mtext+4, 0, strlen(msb.mtext+4));
		if(msgrcv(msqid, &msb, MAXMSG, pid, 0) < 0)
			ERR_EXIT("msgrcv");
		fputs(msb.mtext+4, stdout);
		memset(msb.mtext+4, 0, strlen(msb.mtext+4));
	}
}
