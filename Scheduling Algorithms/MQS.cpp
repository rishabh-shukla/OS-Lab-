#include <iostream>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <signal.h>
#include <climits>
#include <semaphore.h>

using namespace std;

#define TABLESZ 100
#define READYQSZ 100
#define MAX 10000
#define MAXQ 10

int schID;
int q[MAXQ];	//time quantum
int nq;	//number of queues

void timeUsage(float s)
{
    int sec = int(s*1000000);
    usleep(sec);
}

struct processInf{
	bool valid;
	int runTime;	//Time it has executed
	int execTime;	//Execution Time left
	int pID;
};

sem_t processWait;
sem_t *processCount;
sem_t *tableAccess;

timespec st,en;
double runtime;

struct sharedMemory{
	int rdT, wtT;
	int rdQ[MAXQ], wtQ[MAXQ];
	int pix[MAX];
	int* pExec[MAX];	
	int xy[MAX];
	int qix[MAX];
	int tim[TABLESZ];
	processInf pTable[TABLESZ];
	processInf readyQ[MAXQ][READYQSZ];
};

void signalHandler(int sgid){
    if(sgid == SIGUSR1)
        sem_wait(&processWait);
    else
        sem_post(&processWait);
}

void spawnProcess(int &x, int &tm, int &ix, int &tim){

	clock_gettime(CLOCK_MONOTONIC,&st);

	pid_t pid;

	pid = fork();

	if(pid == 0){
		while(1){	
			if(tm<=q[ix]){
				printf("Child process started executing.\n");
				timeUsage(tm);
				printf("Child process executed completely.\n");
				tm = 0;
				clock_gettime(CLOCK_MONOTONIC,&en);
				runtime = en.tv_sec - st.tv_sec+(en.tv_nsec-st.tv_nsec)/(1e9);
				tim += (int)runtime;
				kill(schID, SIGUSR2);
				exit(0);
			}
			else{
				printf("Child process started executing.\n");
				timeUsage(q[ix]);
				printf("Child process executed partially.\n");
				tm -= q[ix];
				kill(schID, SIGUSR2);
				kill(getpid(), SIGUSR1);
			}	
		}
	}
	else{
		kill(pid, SIGUSR1);
		x = pid;
	}
}

void processInjector(int n){

    sem_t *processCount = sem_open("/processCount", 0);

    sem_t *tableAccess = sem_open("/tableAccess", 0);

    int segment_id;
	sharedMemory *shMem;

	segment_id = shmget(5678, sizeof(sharedMemory), 0666);

	shMem = (sharedMemory*) shmat(segment_id, NULL, 0);
	
	//0-indexed arrays	
	
	shMem->rdT = 0;		
	shMem->wtT = 0;
	for(int i=0;i<nq;i++){
		shMem->rdQ[i] = 0;
		shMem->wtQ[i] = 0;
	}	

	for(int i=1;i<=n;i++){
        timeUsage(1);
		
		processInf tmp;
		
		tmp.runTime = 0;
		tmp.execTime = rand()%10 + 1;
		
		//Table Access start		
		sem_wait(tableAccess);		
		shMem->xy[i-1] = tmp.execTime;
		shMem->qix[i-1] = 0;
		shMem->tim[i-1] = -1*tmp.execTime;
	
		spawnProcess(tmp.pID, shMem->xy[i-1], shMem->qix[i-1], shMem->tim[i-1]);

		(shMem->pExec[tmp.pID]) = &(shMem->xy[i-1]);
	
		shMem->pix[tmp.pID] = i - 1;		//  Process Index is i	

		cout<<"Injection :: "<<tmp.pID<<" "<<tmp.execTime<<endl;

		sem_post(processCount);		
		
		tmp.valid = 1;

		shMem->pTable[shMem->wtT++] = tmp;
		
		shMem->readyQ[0][shMem->wtQ[0]++] = tmp;
		shMem->wtQ[0] %= READYQSZ;

		sem_post(tableAccess);
		//Table access end

	}
	

	shmdt(shMem);
    exit(0);

}

void MQS(int n){

    sem_t *processCount = sem_open("/processCount", 0);

    sem_t *tableAccess = sem_open("/tableAccess", 0);

    int segment_id;

	sharedMemory *shMem;

	segment_id = shmget(5678, sizeof(sharedMemory), 0666);

	//cout<<segment_id<<endl;

	shMem = (sharedMemory*) shmat(segment_id, NULL, 0);

	int cnt = 0;

	while(cnt<n){
		
		processInf tmp;
		
        sem_wait(processCount);

		//Table Access start			
		sem_wait(tableAccess);
		
		int i = 0;
		while(shMem->rdQ[i] == shMem->wtQ[i])
			i++;

		tmp = shMem->readyQ[i][shMem->rdQ[i]++];
		shMem->rdQ[i] %= READYQSZ;

		sem_post(tableAccess);
		//Table access end		

		cout<<"Scheduled :: "<<tmp.pID<<endl;	

		kill(tmp.pID, SIGUSR2);
		
		kill(getpid(), SIGUSR1);	

		//Table Access start		
		sem_wait(tableAccess);
		
		cout<<"Runtime of process left :: "<<tmp.pID<<" " <<*(shMem->pExec[tmp.pID])<<endl;		
		
		tmp.runTime = tmp.execTime - *(shMem->pExec[tmp.pID]);
		tmp.execTime = *(shMem->pExec[tmp.pID]);
		
		shMem->pTable[shMem->pix[tmp.pID]] = tmp;		
		
		if(*(shMem->pExec[tmp.pID]) == 0){
			shMem->pTable[shMem->pix[tmp.pID]].valid = 0, cnt++;
			cout<<"Waiting Time :: "<<shMem->tim[shMem->pix[tmp.pID]]<<endl;
		}
		else{
			if(i != nq-1){
				shMem->readyQ[i+1][shMem->wtQ[i+1]++] = tmp;
				shMem->wtQ[i+1] %= READYQSZ;
				shMem->qix[shMem->pix[tmp.pID]]++;
			}	
			else{
				shMem->readyQ[i][shMem->wtQ[i]++] = tmp;
				shMem->wtQ[i] %= READYQSZ;
			}
			sem_post(processCount);
		}
			
		sem_post(tableAccess);
		//Table access end

	}

	shmdt(shMem);
	exit(0);

}

int main(){

    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);

    sem_t *sem1 = sem_open("/processCount", O_CREAT, 0644, 0);

    sem_t *sem2 = sem_open("/tableAccess", O_CREAT, 0644, 0);
    sem_post(sem2);

    sem_init(&processWait, 0, 0);

	int segment_id;

	segment_id = shmget(5678, sizeof(sharedMemory), IPC_CREAT | 0666);


	int n;

	printf("Enter the number of processes :: \n");
	scanf("%d", &n);

	printf("Enter the number of queues :: \n");
	scanf("%d", &nq);

	printf("Enter the value of time quantum's of each queue :: \n");
	for(int i=0;i<nq;i++)	
		scanf("%d", &q[i]);

	pid_t schedulerID;

	schedulerID = fork();

	if(schedulerID == 0){
		MQS(n);
	}
	else{
		
		schID = schedulerID;		

		pid_t processInjectorID;

		processInjectorID = fork();

		if(processInjectorID == 0){
			processInjector(n);
		}
		else{			
			waitpid(processInjectorID, 0, 0);
			waitpid(schedulerID, 0, 0);	

			sem_unlink("/processCount");
			sem_unlink("/tableAccess");
			
            shmctl(segment_id, IPC_RMID, NULL);
		}
	}

	return 0;
}
