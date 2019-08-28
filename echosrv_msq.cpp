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

void echsrv_msq()
{
	int msqid = msgget(1234, 0666 | IPC_CREAT);
	if(msqid < 0) ERR_EXIT("msgget");
	struct msgbuf msb;
	memset(&msb, 0, sizeof(struct msgbuf));
	while(1)
	{
		int nrcv;
		if((nrcv = msgrcv(msqid, &msb, MAXMSG, 1, 0)) < 0)
			ERR_EXIT("msgrcv");
		fputs(msb.mtext+4, stdout);
		msb.mtype = *((int*)msb.mtext);
		if(msgsnd(msqid, &msb, nrcv, 0) < 0)
			ERR_EXIT("msgsnd");
		memset(msb.mtext, 0, nrcv);
	}
}
