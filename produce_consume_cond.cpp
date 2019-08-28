#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFSIZE 10
#define PRODUCER 2
#define CONSUMER 1

pthread_t g_buff[PRODUCER+CONSUMER];
pthread_mutex_t mutex_t;
pthread_cond_t cond_t;

int nready = 0;

void* produce(void *arg)
{
	while(1)
	{
		pthread_mutex_lock(&mutex_t);
		printf("%0x produce product...\n", (int)pthread_self());
		nready++;
		pthread_cond_signal(&cond_t);
		pthread_mutex_unlock(&mutex_t);
		sleep(2);
	}
	return NULL;
}

void* consume(void *arg)
{
	while(1)
	{
		pthread_mutex_lock(&mutex_t);
		printf("%0x consume product...\n", (int)pthread_self());
		while(nready == 0)
		{
			printf("%0x consume in wait...\n", (int)pthread_self());
			pthread_cond_wait(&cond_t, &mutex_t);
		}
		nready--;
		printf("%0x consume is finshed...\n", (int)pthread_self());
		pthread_mutex_unlock(&mutex_t);
		sleep(2);
	}
	return NULL;
}

int main()
{
	pthread_mutex_init(&mutex_t, NULL);
	pthread_cond_init(&cond_t, NULL);
	int i, errnum;
	for(i=0; i<PRODUCER; ++i)
		if((errnum = pthread_create(&g_buff[i], NULL, produce, NULL)) != 0)
		{
			fprintf(stderr, "error : %s\n", strerror(errnum));
			exit(EXIT_FAILURE);
		}
	for(i=0; i<CONSUMER; ++i)
		if((errnum = pthread_create(&g_buff[i+PRODUCER], NULL, consume, NULL)) != 0)
		{
			fprintf(stderr, "error : %s\n", strerror(errnum));
			exit(EXIT_FAILURE);
		}
	for(i=0; i<PRODUCER+CONSUMER; ++i)
		if((errnum = pthread_join(g_buff[i], NULL)) != 0)
		{
			fprintf(stderr, "error : %s\n", strerror(errnum));
			exit(EXIT_FAILURE);
		}
	pthread_cond_destroy(&cond_t);
	pthread_mutex_destroy(&mutex_t);
	return 0;
}
