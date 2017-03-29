#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<semaphore.h>

sem_t chair;
sem_t FilmFare;
sem_t Max_limit;
sem_t is_finish;
sem_t is_ready;
sem_t is_barber_awake;


void* customer(void* j)
{
	int i = *((int*)j);
	int z;
	printf(">>> ");
	scanf("%d", &z);
	printf("Customer %d waiting outside.\n", i+1);
	sem_wait(&Max_limit);
	printf("Customer %d reading OS book.\n", i+1);
	sem_wait(&FilmFare);
	printf("Customer %d reading FilmFare Magazine.\n", i+1);
	sem_wait(&chair);
	sem_post(&FilmFare);
	printf("Customer %d is on chair.\n", i+1);
	sem_post(&is_ready);
	sem_wait(&is_finish);
	sem_post(&chair);
	printf("Barber finished with customer %d and he will be going to cash counter.\n", i+1);
	sleep((int)(rand()%1) + 2); // customer paying
	printf("Customer %d pay his payment and leave.\n", i+1);
	sem_post(&Max_limit);
	return;
}

void* barber(void* j)
{
	int i = *((int*)j);
	while(1)
	{
		sem_wait(&is_ready);
		sem_wait(&is_barber_awake);
		printf("Barber %d is cutting hair.\n", i+1);
		sleep((int)(rand()%4) + 6); // cutting hair
		sem_post(&is_barber_awake);
		sem_post(&is_finish);
		printf("Barder %d complete his work and now going to sleep again.\n", i+1);
	}
}

int main()
{
	sem_init(&chair, 0, 3);
	sem_init(&FilmFare, 0, 4);
	sem_init(&Max_limit, 0, 10);
	sem_init(&is_finish, 0, 0);
	sem_init(&is_ready, 0, 0);
	sem_init(&is_barber_awake, 0, 0);
	
	int i;
	pthread_t barber_thread[3], customer_thread[50];
	int barber_thread_id[3], customer_thread_id[50];
	for (i=0; i<3; i++)
	{
		barber_thread_id[i] = i;
		sem_post(&is_barber_awake);
		printf("Barber %d is sleeping.\n", i+1);
		pthread_create(&barber_thread[i], NULL, barber, &barber_thread_id[i]);
	}
	
	for (i=0; i<50; i++)
	{
		customer_thread_id[i] = i;
		pthread_create(&customer_thread[i], NULL, customer, &customer_thread_id[i]);
	}
	
	for (i=0; i<50; i++)
	{
		pthread_join(customer_thread[i], NULL);
	}
	
	for (i=0; i<3; i++)
		pthread_join(barber_thread[i], NULL);
	
	return 0;
}

