#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#define NUM_CHILDREN 10
#define TIME_QUANTUM 3

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
int current_pid_idx = -1;
int alive_processes = NUM_CHILDREN;
int system_time = 0;
int global_guantum_setting = TIME_QUANTUM;

void check_and_reset_quantums(){
	bool all_zero = true;
	for(int i=0; i <NUM_CHILDREN; i++){
		if(pcb_table[i].status != DONE && pcb_table[i].time_quantum>0){
			all_zero = false;
			break;
		}
	}
	if (all_aero && alive_processes>0){
		printf("[kernel] all process quantums have been consumed -> reset all quantum \n");
		for (int i=0; i < NUM_CHILDREN; i++){
			if(pcb_table[i].status != DONE){
				pcb_table[i].time_quantum = global_quantum_setting;
		
			}
		}
	}

}

// parent timer - RR scheduler
void parent_timer_handler(int signo){
	system_time++;
	printf("\n --- time tick : %d {alive : %d} ---\n", system_time, alive_processes);

	if(alive_processes == 0) return;

	// checking ready queue process
	for(int i = 0; i < NUM_CHILDREN; i++){
		if (pcb_table[i].status == READY){
			pcb_table[i].waiting_time++;
		}
		else if (pcb_table[i].status == SLEEP){
			pcb_table[i].io_table_remain--;
			if (pcb_table[i].io_wait_remain <= 0){
				// finished waiting time -> move to ready q
				pcb_table[i].status = READY;
				printf("[kernel] child %d wake up from IO -> Ready! \n", pcb_table[i].pid);
			}
		}
	}

	// now running process 
	if (current_pid_idx != -1){
		pcb_t *proc = &pcb_table[current_pid_idx];

		// running process's time quantum --
		// send start signal to child
		// time quantum -> 0 -> change to next process
		// if not 0 -> keepgoing
		//
	}else{
		schedule_next_process();
	}
	check_and_reset_quantums();
}

// io : child to parent
void parent_io_handler(int signo){
	if (current_pid_idx != -1){
		pcb_t *proc = &pcb_table[current_pid_idx];

		//child io request -> allocate waiting time
		proc->io_wait_remain = (rand() % MAX_IO_WAIT) + 1;
		
		//sleep q - changing status
		proc->status = SLEEP;
		
		printf("[kernel] child %d requested IO... wait time: %d. moving to sleep \n", 
				proc->pid, proc->io_wait_remain);
		
		//running process went to sleep -> try scheduleing
		schedule_next_process();	
}

// ch exit
void parent_child_handler(int signo){
	//child exit
	int status;
	pid_t pid;
	while ((pid=waitpid(-1, &status, WNOHANG))>0){
		for (pcb_table[i].pid == pid){
			pcb_table[i].status = DONE;
			alive_processes--;
			printf("[kernel] child %d (pid: %d) terminated. \n", i, pid);

			if (current_pid_idx == i){
				current_pid_idx = -1;
				schedule_next_process();
			}
			break;
		}
	}
}
//for child
int my_cpu_burst = 0;

//child action handler
void child_action_handler(int signo){}

//main child process
void child_process_main(){
	srand(time(NULL)^getpid());

	//sig handler
	struct sigaction sa;
	sa.sa_handler = child_action_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIG_RUN_STEP, &sa, NULL);

	my_cpu_burst = (rand() % MAX_BURST) + 1;

	while(1){
		pause();
	}
}

// rr scheduler
void schedule_next_process(){
	if (alive_processes == 0) return;

	int start_idx = (current_pid_idx + 1) % NUM_CHILDREN;
	int next_idx = -1;

	for(int i=0; i<NUM_CHILDREN; i++){
		int idx = (start_idx + i) % NUM_CHILDREN;
		if (pcb_table[idx].status == READY && pcb_table[idx].time_quantum > 0){
			next_idx = idx;
			break;
		}
	}
	if (next_idx != -1){
		//context switching
		current_pid_idx = next_idx;
		pcb_table[current_pid_idx].status = RUNNING;
		printf("[kernel] switching to child %d (pid : %d\n)", current_pid_idx, pcb_table[current_pid_idx].pid);
	}else{
		//if empty
		current_pid_idx = -1;
		//if quantum is all 0 
		check_and_reset_quantums();
	}
}

//main
int main(){
	//parent sig
	signal(SIGALRM, parent_timer_handler);
	signal(SIGUSR2, parent_io_handler);
	signal(SIGCHLD, parent_chld_handler);
	
	for( int i=0; i<NUM_CHILDREN; i++){
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


