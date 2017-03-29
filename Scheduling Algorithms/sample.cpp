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


int main(){

	int tm = 0;
	if(1){
		tm++;
		cout<<tm<<endl;
		if(tm !=2)
			continue;
	}
	return 0;
}
