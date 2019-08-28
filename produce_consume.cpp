#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFSIZE 10
#define PRODUCER 2
#define CONSUMER 1

int pro_st = 0;
int con_st = 0;
int p_id = 0;
int c_id = 0;
int good_buff[BUFFSIZE];
pthread_t g_buff[PRODUCER+CONSUMER];
sem_t buff_full;
sem_t buff_empty;
pthread_mutex_t mutex_t;

void* produce(void *arg)
{
	int i;
	while(1)
	{
	printf("in thread %0x", (int)pthread_self());
	sem_wait(&buff_full);
	pthread_mutex_lock(&mutex_t);
	good_buff[pro_st] = p_id;
	printf("%0x produce product \n", (int)pthread_self());
	for(i=0; i<BUFFSIZE; ++i)
	{
		if(good_buff[i] == -1)
			printf("%s", "null");
		else if(i == pro_st)
			printf("%d <--", good_buff[i]);
		else printf("%d", good_buff[i]);
		printf("\n");
	}
	pro_st = (pro_st+1) % BUFFSIZE;
	printf("%d product is finshed\n", p_id++);
	pthread_mutex_unlock(&mutex_t);
	sem_post(&buff_empty);
	sleep(3);
	}
	return NULL;
}

void* consume(void *arg)
{
	int i;
	while(1)
	{
	printf("in consum thread %0x", (int)pthread_self());
	sem_wait(&buff_empty);
	pthread_mutex_lock(&mutex_t);
	printf("%0x consumers in consume \n", (int)pthread_self());
	for(i=0; i<BUFFSIZE; ++i)
	{
		if(good_buff[i] == -1)
			printf("%s\n", "null");
		else if(i == con_st)
			printf("%d -->\n", good_buff[i]);
		else printf("%d\n", good_buff[i]);
		printf("\n");
	}
	good_buff[con_st] = -1;
	con_st = (con_st+1) % BUFFSIZE;
	printf("%d consume is finshed\n", c_id++);
	pthread_mutex_unlock(&mutex_t);
	sem_post(&buff_full);
	sleep(3);
	}
	return NULL;
}

int main()
{
	sem_init(&buff_full, 0, 10);
	sem_init(&buff_empty, 0, 0);
	pthread_mutex_init(&mutex_t, NULL);
	int i, errnum;
	for(i=0; i<BUFFSIZE; ++i)
		good_buff[i] = -1;
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
	sem_destroy(&buff_full);
	sem_destroy(&buff_empty);
	pthread_mutex_destroy(&mutex_t);
	return 0;
}
