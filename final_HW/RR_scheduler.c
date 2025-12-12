#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#define NUM_CHILDREN 10
#define TIME_Q 3

//pcb structure
typedef struct{
	pid_t pid;
	int time_q;
	int io_wait;
	ProcessState status;
	int waiting;
} pcb_t;

//pcb status
typedef enum { READY, RUNNING, SLEEP, DONE} ProcessStatus;

pcb_t pcb_table[NUM_CHILDREN];

// parent timer - RR scheduler
void parent_timer_handler(int signo){}
// io ch to pa
void parent_io_handler(int signo){}
// ch exit
void parent_child_handler(int signo){
	//child exit
}
//child action handler
void child_action_handler(int signo){}

// main child process
void child_process_main(){
	//child sig
	signal(SIGUSR1, child_action_handler);
	printf("[child %d] ready \n", getpid());
	while(1){
		pause();
	}
}

//main
int main(){
	//parent sig
	signal(SIGALRM, parent_timer_handler);
	signal(SIGUSR2, parent_io_handler);
	signal(SIGCHLD, parent_chld_handler);
	
	for( int i=0; i<NUM_CHILDRENl; i++){
		pid_t pid = fork();

		if (pid==0){
			// start main child
			child_process_main();
			exit(0);
		}else if(pid>0){
			pcb_table[i].pid = pid;
			pcb_table[i].status = READY;
			pcb_table[i].time_q = TiME_Q;
		}else{
			perror("fork failed");
		}
	}

	alarm(1);
	printf("-------");

	while(1){
		pause();
		alarm(1);
	}
	return 0;
}


