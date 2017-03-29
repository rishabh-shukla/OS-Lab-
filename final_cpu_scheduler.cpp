
#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <limits.h>

using namespace std;

int *np; // number of processes	/*shared memory*/

bool *new_entry; 	/*shared memory*/

sem_t mutex;	// mutex for each processes

// injects the process in the system
void ProcessInjector ();

// scheduler to run injected processes in particular order
void Scheduler ();

// signal handler for process to block
void WaitSH (int);

// signal handler for process to unblock
void WakeSH (int);

// function to be runned by process
void ProcessFunc (int);

inline double getTime (timespec t) {
	return ((t.tv_sec * 1000) + (t.tv_nsec / 1000000)) + 0.5;
}
// process structure /*shared memory*/
struct process_struct {
	double wt;	// waiting time
	double iwt;	// initial system time for waiting time
	double turn; // turn around time
	int ti;	// time required to process
	int pid; // process id
}*process; // process table

// linked list node for queue
struct node{
    int info;
    struct node *next;
};

// for multilevel queue feedback schedular algorithm
class queue{
    private:
        node *rear;
        node *front;
    public:
        queue() {
        	rear = NULL;
        	front = NULL;
        }
        void enqueue(int data) {
		    node *temp = new node;
		    temp->info = data;
		    temp->next = NULL;
		    if(front == NULL) {
		        front = temp;
		    }else {
		        rear->next = temp;
		    }
		    rear = temp;
        }
        int dequeue() {
    	    node *temp = new node;
    	    temp = front;
    	    front = front->next;
    	    int ret = temp->info;
    	    delete temp;
    	    return ret;
		}
        void display() {
        	if(front == NULL) {
        		cout << "empty";
        		return;
        	}
        	node *p = new node;
		    p = front;
		    while(p != NULL){
		        cout << p->info+1 << " ";
		        p = p->next;
		    }
		    cout<<endl;
        }

        bool empty() {
        	return front==NULL;
        }

};

// main
int main () {
	// making signal SIGUSR1 for wait 
	signal(SIGUSR1,WaitSH);
	// making signal SIGUSR2 for wake
	signal(SIGUSR2,WakeSH);
	
	// number of processes
	np = (int*)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
	// new entry comes
	new_entry = (bool*)mmap(NULL, sizeof(bool), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);

	cout << "Enter number of Processes : ";
	cin >> *np;

	// process table
	process = (process_struct*)mmap(NULL, sizeof(process_struct)* (*np), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);

	// random function for ti values
	time_t randomt;
	time(&randomt);	

	// input for value for Ti number
	for(int i = 0; i < *np; i++) {
		//cout<<"Enter number Ti for process "<<i+1<<" : ";
		//cin>>ti[i];
		process[i].ti = 10000 + rand() % 10000;;
		cout << "Process " << i+1 << " ti : " << process[i].ti << endl;
	}
	
	int pid = fork();
	// making process that executes ProcessInjector
	if(pid == 0) {
		ProcessInjector();
		return 0;
	}

	
	pid = fork();
	// making process that executes Scheduler
	if(pid == 0) {
		Scheduler();
		return 0;
	}


	cout<<"Both processes made\n";
	// wait for both the processes to complete
	wait(0);
	wait(0);
	return 0;
}

// defination for projectinjector 
void ProcessInjector() {
    int pid;
    for (int i = 0; i < *np; i++) {
        pid = fork();
        // making all processes
        if (pid == 0) {
        	// initialization of the mutex of particular process
	    	sem_init(&mutex,0,0);
	   		// run process function
           	ProcessFunc(i);
            return;
        }
        process[i].pid = pid;
        // blocking process
        kill(pid,SIGUSR1);
        *new_entry = true;
		cout << "----------------PROCESS "<<i+1<<" ARRIVED----------------" << endl;
		sleep(1.01); // arrival time of the process
    }
    // wait for all the processes to completes
    for(int i = 0; i < *np; i++) {
    	wait(0);
    }
}

// defination for scheduler
void Scheduler() {
	int ch;
	cout << "\nMENU FOR CPU SCHEDULING\n";
	cout << "1.FCFS method\n";
	cout << "2.SHORTEST JOB FIRST method(non-preemptive)\n";
	cout << "3.SHORTEST JOB FIRST method(preemptive)\n";
	cout << "4.ROUND ROBIN method\n";
	cout << "5.PRIORITY method \n";
	cout << "6.Multilevel Feedback Queue method\n";
	cout << "7.EXIT\n";
	cout << "ENTER YOUR CHOICE : \n";
	cin >> ch;
	// variables for real start time and end time of the algorithm
	timespec et,st;
   	
	switch(ch) {
		// FCFS method
		case 1: {
				// start time of the algorithm
				clock_gettime(CLOCK_REALTIME, &st);
				for(int i = 0; i < *np; i++) {
						process[i].iwt = getTime(st);
				}
				// checks if all the processes is completed
				int incomplete = *np;
				while(incomplete) {
					for(int i = 0; i < *np; i++) {
						if(process[i].pid > 0) {
							// unblocking process
							kill(process[i].pid, SIGUSR2);
							// wait for process to complete
							while(kill(process[i].pid,0)==0);
							// update table
							process[i].pid = -1;
							incomplete--;
						}
					}
				}
		break;
		}
		// Shortest Job First method (non-preemptive)
		case 2: {
			// get real start time of the algorithm
			clock_gettime(CLOCK_REALTIME, &st);
			for(int i = 0; i < *np; i++) {
				process[i].iwt = getTime(st);
			}
			// checks if all the processes is completed
			int incomplete = *np;
			while(incomplete) {
				// selecting shortest job in table
				int min = INT_MAX;
				int index = -1;
				for(int i = 0; i < *np; i++) {
					if(process[i].pid > 0 && min > process[i].ti) {
						min = process[i].ti;
						index = i;
					}
				}
				if(index != -1) {
					// unblocking process
					kill(process[index].pid, SIGUSR2);
					// wait for process to complete
					while(kill(process[index].pid, 0)==0);
					// update table
					process[index].pid = -1;
					incomplete--;
				}
			}
		break;
		}
		// Shortest Job First method (preemptive)
		case 3 : {
			// get real start time of algorithm
			clock_gettime(CLOCK_REALTIME, &st);
			for(int i = 0;i< *np; i++) {
				process[i].iwt = getTime(st);
			}
			// checks if all the processes is completed
			int incomplete = *np;
			// while there is a process in the table
			while(incomplete) {
				cout<<"----------\n";
				for(int i = 0;i< *np; i++) {
					if(process[i].pid>0){	
						cout<<"ti value for process "<<i+1<<" : "<<process[i].ti<<endl;
					}
				}
				cout<<"----------\n";
				// no new entry
				*new_entry = false;
				int min = INT_MAX;
				int index = -1;
				// selecting shortest job in table
				for(int i = 0;i < *np; i++) {
					if(process[i].pid > 0 && min > process[i].ti ) {
						min = process[i].ti;
						index = i;
					}
				}
				if(index != -1) {
					cout<<"                 Process "<<index+1<<" picked"<<endl;
					// unblocking of process
					kill(process[index].pid, SIGUSR2);
					// wait for process to complete or new entry
					while(kill(process[index].pid,0)==0 && !(*new_entry));
					// process is completed or array sorted
					if(kill(process[index].pid, 0)!=0 || process[index].ti==0) {
						// remove it
						process[index].pid = -1;
						incomplete--;
					} else {
						// block it
						kill(process[index].pid,SIGUSR1);
					}
				}

			}
		break;
		}
		// Round Robin method
		case 4 : {
			// time quantum
			double tq;
			timespec ts,te;
			cout<<"Enter time-quantum in millisecs: ";
			cin>>tq;
			// starting time of algorithm
			clock_gettime(CLOCK_REALTIME, &st);
			for(int i=0;i<*np;i++) {
				process[i].iwt = getTime(st);
			}
			int index = -1;
			// checks if all processes is completed
			int incomplete = *np;
			while(incomplete) {
				//cout << flush;
				cout<<"\n\n----------\n";
				for(int i=0;i<*np;i++) {
					cout<<"ti process "<<i+1<<" : "<<process[i].ti<<"\n";
				}
				cout<<"----------\n";

				for(int i = 0;i< *np; i++) {					
					index = (index+1)%(*np);
					if(process[index].pid>0) {
						break;
					}
				}
				if(index!=-1) {
					cout<<"               Process "<<index+1<<" picked\n";
					kill(process[index].pid,SIGUSR2);
					// starting time of the process
	   				clock_gettime(CLOCK_REALTIME, &ts);
	   				// ending time of the process
	   				clock_gettime(CLOCK_REALTIME, &te);
	   				// waits for process to complete or exceeds given time quantum
					while(getTime(te)-getTime(ts)<=tq && kill(process[index].pid,0)==0) {
	   					clock_gettime(CLOCK_REALTIME, &te);
					}
					// if process not alive
					if(kill(process[index].pid,0)!=0 || process[index].ti==0) {
						// remove from table
						process[index].pid = -1;
						incomplete--;
					} else {
						// block it
						kill(process[index].pid,SIGUSR1);
					}
				}
			}
		break;		
		}
		// Priority method
		case 5 : {
			int pri[*np];
			// getting input for priority values
			for(int i = 0; i < *np; i++) {
				cout<<"Enter priority for process "<<i+1<<" : ";
				cin>>pri[i];
			}
			// starting time of the algorithm
			clock_gettime(CLOCK_REALTIME, &st);
			for(int i = 0;i < *np; i++) {
				process[i].iwt = getTime(st);
			}
			// checks if all processes is completed
			int incomplete = *np;
			while(incomplete) {
				// picking highest priority valued process
				int index = -1;
				int most_priority = INT_MIN;
				for(int i = 0; i < *np; i++) {
					if(process[i].pid>0 && most_priority < pri[i]) {
						most_priority = pri[i];
						index = i;
					}
				}
				if(index!=-1) {
					pri[index] = -1;
					// unblock the process
					kill(process[index].pid,SIGUSR2);
					// waits for the process to complete
					while(kill(process[index].pid,0)==0);
					// remove from table
					process[index].pid = -1;
					incomplete--;
				}
			}
		break;
		}
		// Multilevel Queue Feekback method
		case 6 : {
			// total levels of queue
			int *levels;	/*shared memory*/
			levels = (int*)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
			cout<<"How many levels do you want (such a way that every queue level fills at some instant) ? : ";
			cin>>*levels;
			// time quantum for each level-1
			int *tq;		/*shared memory*/
			tq = (int*)mmap(NULL, sizeof(int)*(*levels-1), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
			// queues for all levels
			queue *q;	/*shared memory*/
			q = (queue*)mmap(NULL, sizeof(queue)*(*levels), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
			// processes id of queues
			int *qid;	/*shared memory*/
			qid = (int*)mmap(NULL, sizeof(int)*(*levels), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);

			cout<<"Here base level "<<*levels<<" will be FCFS\n";
			// getting input for all levels -1
			for(int i = 0; i < *levels-1;i++) {
				cout<<"Enter Time Quantum for level "<<i+1<<" (in millisec & increasing order) : ";
				cin>>tq[i];
			}
			cout<<"************************************************************************************\n";
			// putting all the process in level 1
			for(int i = 0; i < *np;i++) {
				q[0].enqueue(i);
			}
			clock_gettime(CLOCK_REALTIME, &st);
			for(int i = 0; i < *np; i++) {
				process[i].iwt = getTime(st);
			}

			for(int i = 0;i < *levels;i++) {
				pid_t pid = fork();
				// start making queues for next level
				if(pid == 0) {
					int currlevel = i;
					// while previous level queue is running or current level is not empty
					while((currlevel-1>=0 && kill(qid[currlevel-1],0)==0) || !q[currlevel].empty()) {
						if(!q[currlevel].empty()) {
							cout<<"\n\nINI Level "<<currlevel+1<<" : "; q[currlevel].display(); 
						}
						if(!q[currlevel].empty()) {
							int index = q[currlevel].dequeue();
							cout<<"Process "<<index+1<<" is poped at level "<<currlevel+1<<endl;
							// unblock the process
							kill(process[index].pid,SIGUSR2);
							// current level != last level
							if(currlevel!=*levels-1) {
								timespec ts,te;
								// start time
				   				clock_gettime(CLOCK_REALTIME, &ts);
				   				// end time
				   				clock_gettime(CLOCK_REALTIME, &te);
				   				// waits for process to exceed time quantum
								while(getTime(te)-getTime(ts)<=tq[currlevel] && kill(process[index].pid,0)==0) {
				   					clock_gettime(CLOCK_REALTIME, &te);
								}
								if(kill(process[index].pid,0)==0) { // if alive
									cout<<"Process "<<index+1<<" is pushed into level "<<currlevel+2<<endl;
									kill(process[index].pid,SIGUSR1);
									q[currlevel+1].enqueue(index);
								}
							} else {
								while(kill(process[index].pid,0)==0); // FCFS
							}
						}
					}
					return;
				}
				// assigning queue id
				qid[i] = pid;
			}

			// wait for all processes
			for(int i=0;i<*levels;i++) {
				wait(0);
			}

		break;
		}
		// EXIT
		case 7 : {
			cout<<"Bye\n";
			exit(0);
		}
		// WRONG CHOICE
		default : {
			cout<<"Wrong Choice \n";
		}
	}
	// get ending time of the algorithm
	clock_gettime(CLOCK_REALTIME, &et);
	double meanTA = 0,meanWT = 0;
	// printing waiting time for each processes
	for(int i = 0; i < *np; i++) {
		meanWT += process[i].wt/(*np);
		cout<<"Waiting time for process "<<i+1<<" : "<<process[i].wt<<" millisecs"<<endl;
	}
	// printing turn around time for each processes
	for(int i = 0; i < *np; i++) {
		meanTA += process[i].turn/(*np);
		cout<<"Turn Around time for process "<<i+1<<" : "<<process[i].turn<<" millisecs"<<endl;
	}
	// mean waiting time
	cout<<"Mean waiting time : "<<meanWT<<" millisecs"<<endl;
	// mean turn around time
	cout<<"Mean turn around time : "<<meanTA<<" millisecs"<<endl;
	// throughput
	cout<<"Throughput : "<<((*np)/(getTime(et)-getTime(st)))*1000<<" processes/second"<<endl;
}

// defination for wait signal handler
void WaitSH(int sig) {
	// getting start time for wait of the process
	timespec ts;
   	clock_gettime(CLOCK_REALTIME, &ts);
   	int k;
	for(int i = 0; i < *np; i++) {
		if(process[i].pid==getpid()) {
			process[i].iwt = getTime(ts);
			k = i;
			break;
		}
	}

	cout<<"Process "<<k+1<<": blocked at ti "<<process[k].ti<<"\n";
	sem_wait(&mutex);
}

// defination for waking up signal handler
void WakeSH(int sig) {
	// getting end time for wait of the process
	timespec ts;
   	clock_gettime(CLOCK_REALTIME, &ts);
   	int k;
	for(int i = 0; i < *np; i++) {
		if(process[i].pid==getpid()) {
			process[i].wt += getTime(ts)-process[i].iwt;
			k=i;
			break;
		}
	}
	cout<<"Process "<<k+1<<": unblocked at ti "<<process[k].ti<<"\n";
	sem_post(&mutex);
}

// defination for function runned by processes
void ProcessFunc(int id) {
	timespec s_tu,e_tu;
   	clock_gettime(CLOCK_REALTIME, &s_tu);
	cout<<"Process "<<id+1<<": started\n";
	time_t randomt;
	time(&randomt);
	srand((unsigned int) randomt);
	int arr[process[id].ti];
	// input numbers into array randomly
	for(int i = 0;i < process[id].ti; i++) {
		arr[i] = rand()%process[id].ti;
	}
	int n = process[id].ti;
	// sort array
	for(int i = 0;i < n; i++) {
		for(int j = i+1; j < n; j++) {
			if(arr[i]>arr[j]) {
				int temp = arr[i];
				arr[i] = arr[j];
				arr[j] = temp;
			}
		}
		process[id].ti = process[id].ti-1;
	}
   	clock_gettime(CLOCK_REALTIME, &e_tu);
   	// calculating turn around time
	process[id].turn = getTime(e_tu)-getTime(s_tu);
	cout<<"****************Process "<<id+1<<": done*****************\n\n";
}
