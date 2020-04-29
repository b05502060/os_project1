#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sched.h>
// #include <linux/ktime.h>
// #include <linux/timekeeping.h>

//#define time_unit  { volatile unsigned long i; for(i=0;i<1/*000000UL*/;i++)printf("a\n");}
#define time_unit  { volatile unsigned long i; for(i=0;i<1000000UL;i++); }
#define swap(x,y)	{int tmp=x;x=y;y=tmp;}
#define timequa 500

struct process{
	char N[33];
	short done;
	unsigned int R;
	unsigned int T;
	pid_t PID;
	struct timespec start;
	struct timespec end;
};

void FIFO(struct process *p,int n);
void RR(struct process *p,int n);
void SJF(struct process *p,int n);
void PSJF(struct process *p,int n);

// static void sig_handler(int sig){
// 	if(sig==SIGCHILD){
// 		printf("CATCH SIGNAL PID=%d\n",getpit());
// 	}
// };

int main(int argc,char *argv[]){ //1,2
	//open file
	printf("input file : %s\n",argv[1]);
	printf("output file : %s\n",argv[2]);
	FILE *fin;
	fin=fopen(argv[1],"r");
	char S[5];
	int n;
	fscanf(fin,"%s",S);
	fscanf(fin,"%d",&n);
	struct process *p = (struct process *)malloc(n*sizeof(struct process));
	// initialize done
	for (int i = 0; i < n; ++i){
		p[i].done=0;
		fscanf(fin,"%s %u %u",p[i].N,&p[i].R,&p[i].T);
	}
	// printf("%s\n",S);
	// printf("%d\n",n);
	// for (int i = 0; i < n; ++i){
	// 	printf("%s  ",p[i].N);
	// 	printf("%u  ",p[i].R);
	// 	printf("%u\n",p[i].T);
	// }
	// time_unit;
	if (strncmp(S,"FIFO",5)==0){
		FIFO(p,n);
	}
	else if (strncmp(S,"RR",5)==0){
		RR(p,n);
	}
	else if (strncmp(S,"SJF",5)==0){
		SJF(p,n);
	}
	else if (strncmp(S,"PSJF",5)==0){
		PSJF(p,n);
	}
	//output
	FILE *fout;
	fout=fopen(argv[2],"w+b");
	for (int i = 0; i < n; ++i){
		fprintf(fout,"%s %d\n",p[i].N,p[i].PID);
		printf("%s %d\n",p[i].N,p[i].PID);
	}
	return 0;
}

void FIFO(struct process *p,int n){
	cpu_set_t set;
	CPU_ZERO(&set);
	int time=0;
	for (int i = 0; i < n; ++i){
		//select process
		int j;
		for (j = 0; j < n ; ++j){
			if(p[j].done==1) continue;
			if(p[j].R <= time) break;
		}
		// if no processes ready
		if (j == n){ time_unit;  i--;  time++;  continue;}

		int status;
		CPU_SET(1, &set);
		sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
		p[j].PID = fork();
	    switch(p[j].PID){
	        // PID == -1 代表 fork 出錯
	        case -1:
	            perror("fork()");
	            exit(-1);
	        
	        // PID == 0 代表是子程序
	        case 0:
	        	printf("i'm process %d.\n",j);
	        	syscall(333,&p[j].start);
	            for (int k = 0; k < p[j].T; ++k)  time_unit;
	            syscall(333,&p[j].end);
	        	syscall(334,getpid(),&p[j].start,&p[j].end);
	            printf("[Project1] %d %lu.%lu %lu.%lu\n",getpid(),p[j].start.tv_sec,p[j].start.tv_nsec,p[j].end.tv_sec,p[j].end.tv_nsec);
	            exit(-1);
	        
	        // PID > 0 代表是父程序
	        default:
	        	CPU_SET(0, &set);
				sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
	            time+=p[j].T;
	            p[j].done=1;
	            wait(&status);
	    }
	}
}

void RR(struct process *p,int n){
	int time=0;int i=0;
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(0, &set);
	sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
	//create pipe
	int pp[n][2];
	for (int k = 0; k <= n; ++k){
		if (pipe(pp[k])<0){
			perror("fork()");
			exit(1);
		}
	}
	//initialize
	int *readyqueue = (int *)malloc((n+1)*sizeof(int)); 
	int front=0; int back=0; int tmp=0;
	while (i < n){
		//fork ready process
		for (int k = 0; k < tmp; ++k){
			//get process number from queue
			int queuenum = (back-k-1)%(n+1);
			if (queuenum < 0) queuenum += (n+1);
			int j=readyqueue[queuenum];
			if (p[j].done==1){tmp++;continue;}
			p[j].done=1;
			//fork the process
			p[j].PID = fork();
	    	switch(p[j].PID){
		        // PID == -1 代表 fork 出錯
		        case -1:
		            perror("fork()");
		            exit(-1);
		        
		        // PID == 0 代表是子程序
		        case 0:;
		        //initialize
		        	char inbuf[4];
		        	int isfirst = 1;
		        	int l;
		        	char buf[4]; buf[01]='1';
		        	int exec=p[j].T;
		        	while(exec>0){
		        		CPU_SET(2, &set);
						sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
						read(pp[j][0], inbuf, 1);
						printf("I'm process %d.\n",j+1);
						CPU_SET(1, &set);
						sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
						if(isfirst)	{syscall(333,&p[j].start);  isfirst=0;}
						for (l = 0; l < timequa && l < exec; ++l)  time_unit;
						exec -= 500;
						if (exec <= 0){
							syscall(333,&p[j].end);
			        		syscall(334,getpid(),&p[j].start,&p[j].end);
				            printf("[Project1] %d %lu.%lu %lu.%lu\n",getpid(),p[j].start.tv_sec,p[j].start.tv_nsec,p[j].end.tv_sec,p[j].end.tv_nsec);
						}
						sprintf(buf, "%d", l);
						write(pp[n][1], buf, 4);
		        	}
		        	exit(-1);
		        
		        // PID > 0 代表是父程序
		        default:;
		            //printf("child was born.\n");
		    }
		}
		// process control
		if (front != back){
			char buf[4];
			int pop = readyqueue[front];
			write(pp[pop][1], "1",1);
			//printf("unlock process %d.\n",pop+1);
			read(pp[n][0],buf,4);
			//convert str to int
			int timeplus;
			sscanf(buf,"%d",&timeplus);
			time+=timeplus;
			//select ready process
			tmp=0;
			for (int k = 0; k < n ; ++k){
				if(p[k].done==1) continue;
				if(p[k].R <= time) {readyqueue[back]=k;  back++;  back=back%(n+1);  tmp++;}
			}
			//adjust readyqueue
			p[pop].T -= 500;
			//i can't use p[readyqueue[front]].T to judge
			int strange = p[pop].T;
			if(strange > 0) {
				//printf("move %d to back %d\n",pop,back);
				readyqueue[back] = pop;
				back = (back+1)%(n+1);
				front = (front+1)%(n+1);
			}
			else{
				front = (front+1)%(n+1);
				i++;
			}
			continue;
		}
		//select ready process
		tmp=0;
		for (int k = 0; k < n ; ++k){
			if(p[k].done==1) continue;
			if(p[k].R <= time) {readyqueue[back]=k;  (back++)%(n+1);  tmp++;}
		}
		time_unit;  time++;
	}
}

void SJF(struct process *p,int n){
	int time=0; 
	cpu_set_t set;
	CPU_ZERO(&set);
	for (int i = 0; i < n; ++i){
		//select process
		int j=-1; int len=-1;
		for (int k = 0; k < n ; ++k){
			if(p[k].done==1) continue;
			if(p[k].R <= time && (p[k].T<len || len==-1)) {len = p[k].T;  j=k;}
		}
		// if no processes ready
		if (j == -1){ time_unit;  i--;  time++;  continue;}
		CPU_SET(1, &set);
		sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
		int status;
		p[j].PID = fork();
	    switch(p[j].PID){
	        // PID == -1 代表 fork 出錯
	        case -1:
	            perror("fork()");
	            exit(-1);
	        
	        // PID == 0 代表是子程序
	        case 0:
	        	printf("i'm process %d.\n",j);
	            syscall(333,&p[j].start);
	            for (int k = 0; k < p[j].T; ++k)  time_unit;
	            syscall(333,&p[j].end);
	        	syscall(334,getpid(),&p[j].start,&p[j].end);
	            printf("[Project1] %d %lu.%lu %lu.%lu\n",getpid(),p[j].start.tv_sec,p[j].start.tv_nsec,p[j].end.tv_sec,p[j].end.tv_nsec);
	            exit(-1);
	        
	        // PID > 0 代表是父程序
	        default:
		        CPU_SET(0, &set);
				sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
	            time+=p[j].T;
	            p[j].done=1;
	            wait(&status);
	    }
	}
}

void PSJF(struct process *p,int n){
	int time=0;int i=0;
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(0, &set);
	sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
	//create pipe
	int pp[n][2];
	for (int k = 0; k <= n; ++k){
		if (pipe(pp[k])<0){
			perror("fork()");
			exit(1);
		}
	}
	//initialize
	int readysort[n];
	readysort[0]=0;
	for (int i = 1; i < n; ++i){
		readysort[i]=i;
		for (int j = i; j > 0; j--){
			if(p[readysort[j-1]].R > p[readysort[j]].R) swap(readysort[j-1],readysort[j]);
		}
	}
	int childnum = 0;
	while (i < n){
		//select and fork ready process
		for (int j = 0; j < n ; ++j){
			if(p[j].done==1 || p[j].R > time) continue;
			childnum++;
			//fork the process
			p[j].done = 1;
			p[j].PID = fork();
	    	switch(p[j].PID){
		        // PID == -1 代表 fork 出錯
		        case -1:
		            perror("fork()");
		            exit(-1);
		        
		        // PID == 0 代表是子程序
		        case 0:;
		        	//initialize
		        	int isfirst = 1;
		        	char inbuf[11];  int runtime;
		        	int k;
		        	char buf[11]; buf[01]='1';
		        	int exec=p[j].T;
		        	while(exec>0){
		        		CPU_SET(2, &set);
						sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
						read(pp[j][0], inbuf, 10);
						sscanf(inbuf,"%d",&runtime);
						printf("I'm process %d.\n",j+1);
						CPU_SET(1, &set);
						sched_setaffinity(0 , sizeof(cpu_set_t) , &set);
						if(isfirst)	{syscall(333,&p[j].start);  isfirst=0;}
						for (k = 0; k < runtime; ++k)  time_unit;
						exec -= runtime;
						if (exec <= 0){
							syscall(333,&p[j].end);
			        		syscall(334,getpid(),&p[j].start,&p[j].end);
				            printf("[Project1] %d %lu.%lu %lu.%lu\n",getpid(),p[j].start.tv_sec,p[j].start.tv_nsec,p[j].end.tv_sec,p[j].end.tv_nsec);
						}
						sprintf(buf, "%d", k);
						write(pp[n][1], buf, 10);
		        	}
		        	exit(-1);
		        
		        // PID > 0 代表是父程序
		        default:;
		            //printf("child was born.\n");
		    }
		}
		// process control
		if (childnum > 0){
			//choose the ready and shortest process
			int running = -1;
			for (int j = 0; j < n; ++j){
				//printf("p[%d].T  %d\n",j,p[j].T);
				if (p[j].R <= time && p[j].T!=0 && (running == -1 || p[j].T < p[running].T))  {running = j;}
			}
			//calculate runtime
			int runtime = -1;	int tmp;
			for (int j = 0; j < n; ++j){
				if(j == running || p[j].T==0)  continue;
				tmp = p[j].R-time;
				if (tmp < 0) tmp=0;
				//test preemptive :ture for no preemptive
				if (p[j].T >= p[running].T - tmp)  tmp = p[running].T;
				//preemptive and the shortest
				if(runtime > tmp || runtime == -1) runtime = tmp;
			}
			if(runtime == -1) runtime = p[running].T;
			//printf("running %d runtime %d\n",running,runtime);
			char buf[11];
			sprintf(buf, "%d", runtime);
			write(pp[running][1], buf, 10);
			//printf("unlock process %d.\n",running+1);
			read(pp[n][0],buf,10);
			//convert str to int
			int timeplus;
			sscanf(buf,"%d",&timeplus);
			time += timeplus;
			//adjust T
			p[running].T -= runtime;
			if (p[running].T == 0){
				childnum--;
				i++;
			}
			continue;
		}
		//no process ready
		time_unit;  time++;
	}
}