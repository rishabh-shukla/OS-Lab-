/*

Use large queue and table size
sort from rd end to wt end

*/
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

timespec st,en;
double runtime;

int timeUsage(float s)
{
    int sec = int(s*1000000);
    return usleep(sec);
}

void signalHandler(int sgid){
	timeUsage(1);    
}

int main(){
	
	signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);
	sem_t sem;
	sem_init(&sem, 0, 5);
	//sem_init(&sem, 0, 0);

	sem_wait(&sem);
	
	pid_t pid;

	pid	= fork();

	if(pid == 0){
		clock_gettime(CLOCK_MONOTONIC,&st);
		cout<<timeUsage(5)<<endl;
		clock_gettime(CLOCK_MONOTONIC,&en);
		runtime = en.tv_sec - st.tv_sec+(en.tv_nsec-st.tv_nsec)/(1e9);
		printf("%f\n", runtime);
		exit(0);
	}
	else{
		timeUsage(1);
		waitpid(pid, 0, 0);
cout<<kill(pid, SIGUSR1)<<endl;
	}
	
	//clock_gettime(CLOCK_MONOTONIC,&st);	
	//timeUsage(1);

	//clock_gettime(CLOCK_MONOTONIC,&en);
    //runtime = en.tv_sec - st.tv_sec+(en.tv_nsec-st.tv_nsec)/(1e9);
    //printf("%f\n", runtime);

		
	return 0;
}
