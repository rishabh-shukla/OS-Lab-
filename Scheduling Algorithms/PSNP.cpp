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

int schID;

void timeUsage(float s)
{
    int sec = int(s*1000000);
    usleep(sec);
}

struct processInf{
	bool valid;
	int pty;
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
	int rdQP, wtQP;
	int pix[MAX];
	int* pExec[MAX];	
	int xy[MAX];
	int tim[TABLESZ];
	processInf pTable[TABLESZ];
	processInf readyQSP[READYQSZ];
};

void signalHandler(int sgid){
    if(sgid == SIGUSR1)
        sem_wait(&processWait);
    else
        sem_post(&processWait);
}

void spawnProcess(int &x, int &tm, int &tim){

	clock_gettime(CLOCK_MONOTONIC,&st);

	pid_t pid;

	pid = fork();

	if(pid == 0){
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
		kill(pid, SIGUSR1);
		x = pid;
	}
}

bool predP(processInf A, processInf B){
	if(A.pty<B.pty)
		return 1;
	else
		return 0;
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
	shMem->rdQP = 0;
	shMem->wtQP = 0;	
	
	for(int i=1;i<=n;i++){
        //timeUsage(1);
		
		processInf tmp;
		
		tmp.runTime = 0;
		tmp.execTime = rand()%10 + 1;
		tmp.pty = rand()%10 + 1;
		
		//Table Access start		
		sem_wait(tableAccess);		
		shMem->xy[i-1] = tmp.execTime;
		shMem->tim[i-1] = -1*tmp.execTime;

		spawnProcess(tmp.pID, shMem->xy[i-1], shMem->tim[i-1]);

		(shMem->pExec[tmp.pID]) = &(shMem->xy[i-1]);
	
		shMem->pix[tmp.pID] = i - 1;		//  Process Index is i	

		cout<<"Injection :: "<<tmp.pID<<" "<<tmp.execTime<<" "<<tmp.pty<<endl;

		sem_post(processCount);		
		
		tmp.valid = 1;

		shMem->pTable[shMem->wtT++] = tmp;
		//shMem->wtT %= TABLESZ;

		shMem->readyQSP[shMem->wtQP++] = tmp;
		shMem->wtQP %= READYQSZ;

		sort(shMem->readyQSP + shMem->rdQP, shMem->readyQSP + shMem->wtQP, &predP);						
	
		sem_post(tableAccess);
		//Table access end

	}

	shmdt(shMem);
    exit(0);

}

void priorityScheduler(int n){

    sem_t *processCount = sem_open("/processCount", 0);

    sem_t *tableAccess = sem_open("/tableAccess", 0);

    int segment_id;

	sharedMemory *shMem;

	segment_id = shmget(5678, sizeof(sharedMemory), 0666);

	shMem = (sharedMemory*) shmat(segment_id, NULL, 0);


	int cnt = 0;

	while(cnt<n){

		processInf tmp;
		
        sem_wait(processCount);

		//Table Access start			
		sem_wait(tableAccess);

		tmp = shMem->readyQSP[shMem->rdQP++];
		shMem->rdQP %= READYQSZ;

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
			shMem->readyQSP[shMem->wtQP++] = tmp;
			shMem->wtQP %= READYQSZ;	
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

	pid_t schedulerID;

	schedulerID = fork();

	if(schedulerID == 0){
		priorityScheduler(n);
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
