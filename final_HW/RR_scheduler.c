#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define NUM_CHILDREN 10
#define TIME_QUANTUM 7

#define MAX_IO_WAIT 5
#define MIN_IO_WAIT 1
#define MAX_BURST 2
#define MIN_BURST 1

#define SIG_RUN_STEP SIGUSR1
#define SIG_REQ_IO SIGUSR2

//pcb status
typedef enum {
	READY, 
	RUNNING, 
	SLEEP, 
	DONE
} ProcessState;

//pcb structure
typedef struct{
	pid_t pid;
	int time_quantum;
	int io_wait_remain;
	ProcessState status;

	int waiting_time;
	int arrival_time;
} pcb_t;

pcb_t pcb_table[NUM_CHILDREN];
int current_pid_idx = -1;
volatile int alive_processes = NUM_CHILDREN;
int system_time = 0;
int global_quantum_setting = TIME_QUANTUM;

void check_and_reset_quantums(){
	bool all_zero = true;
	for(int i=0; i <NUM_CHILDREN; i++){
		if(pcb_table[i].status != DONE && pcb_table[i].time_quantum>0){
			all_zero = false;
			break;
		}
	}
	if (all_zero && alive_processes>0){
		//printf("[kernel] all process quantums have been consumed -> reset all quantum \n");
		for (int i=0; i < NUM_CHILDREN; i++){
			if(pcb_table[i].status != DONE){
				pcb_table[i].time_quantum = global_quantum_setting;
		
			}
		}
	}

}

// implemented below..
void schedule_next_process();

// parent timer - RR scheduler
void parent_timer_handler(int signo){
	system_time++;
	if(system_time % 10 == 0){
		printf("\n --- time tick : %d {alive : %d} ---\n", system_time, alive_processes);
	}

	if(alive_processes <= 0) return;

	// checking ready queue process
	for(int i = 0; i < NUM_CHILDREN; i++){
		if (pcb_table[i].status == READY){
			pcb_table[i].waiting_time++;
		}
		else if (pcb_table[i].status == SLEEP){
			pcb_table[i].io_wait_remain--;
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
		proc->time_quantum--;

		// send start signal to child
		kill(proc->pid, SIG_RUN_STEP);

		// time quantum -> 0 -> change to next process
		if (proc->time_quantum <= 0){
			printf("[kernel] child %d time quantum expired.\n", proc->pid);
			proc->status = READY;
			schedule_next_process();
		}
		// if not 0 -> keepgoing
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
}
// ch exit
void parent_child_handler(int signo){
	//child exit
	int status;
	pid_t pid;
	while ((pid=waitpid(-1, &status, WNOHANG))>0){
		for(int i=0; i<NUM_CHILDREN; i++){
			if (pcb_table[i].pid == pid){
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
}
//for child
int my_cpu_burst = 0;

//child action handler
void child_action_handler(int signo){
	if (signo== SIG_RUN_STEP) {
		// start -> cpu--
		// cpu == 0 -> process kill or io..
		my_cpu_burst--;
		if (my_cpu_burst <= 0){
			int action = rand()%8;
			if(action < 8){
				// process end
				exit(0);
			}else{
				//or io
				my_cpu_burst = (rand()%MAX_BURST) + 1;
				kill(getppid(), SIG_REQ_IO);

				//ready.. until parent call
			}
		}

	}
}

//main child process
void child_process_main(){
	//random,,
	
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
		printf("[kernel] switching to child %d (pid : %d)\n", current_pid_idx, pcb_table[current_pid_idx].pid);
	}else{
		//if empty
		current_pid_idx = -1;
		//if quantum is all 0 
		check_and_reset_quantums();
	}
}
//print result
void print_result(){
	printf("==================\n");
	printf("result!! \n");
	printf("=================");

	double total_wait = 0;
	for (int i=0; i<NUM_CHILDREN; i++){
		printf("child : %d...%d \n", i, pcb_table[i].waiting_time);
		total_wait += pcb_table[i].waiting_time;
	}
	printf("=====");
	printf("average waiting time : %.2f tick \n", total_wait / NUM_CHILDREN);
	printf("quantum setting: %d\n", global_quantum_setting);
	printf("=====");
}

//main
int main(){
	//parent sig
	struct sigaction sa_alarm, sa_io, sa_child;

	//1. timer signal
	sa_alarm.sa_handler = parent_timer_handler;
	sigemptyset(&sa_alarm.sa_mask);
	sa_alarm.sa_flags = 0;
	sigaction(SIGALRM, &sa_alarm, NULL);

	//2. io request signal
	sa_io.sa_handler = parent_io_handler;
	sigemptyset(&sa_io.sa_mask);
	sa_io.sa_flags = 0;
	sigaction(SIG_REQ_IO, &sa_io, NULL);

	//3. child exit signal
	sa_child.sa_handler = parent_child_handler;
	sigemptyset(&sa_child.sa_mask);
	sa_child.sa_flags = SA_RESTART; // restart waitpid..
	sigaction(SIGCHLD, &sa_child, NULL);

	srand(time(NULL));

	// make 10 child proc.. pcb reset
	printf("[kernel] creating %d children... \n", NUM_CHILDREN);
	
	for( int i=0; i<NUM_CHILDREN; i++){
		pid_t pid = fork();

		if (pid==0){
			// start main child
			child_process_main();
			exit(0);
		}else if(pid>0){
			//reset pcb (parent)
			pcb_table[i].pid = pid;
			pcb_table[i].time_quantum = TIME_QUANTUM;
			pcb_table[i].status = READY;
			pcb_table[i].arrival_time = 0;
			pcb_table[i].waiting_time = 0;
			pcb_table[i].io_wait_remain = 0;
		}else{
			perror("fork failed");
			exit(1);
		}
	}
	
	// start timer... 0.01 sec
	printf("[kernel] simulation start.. quantum is %d.. -kjy\n", global_quantum_setting);
	ualarm(50000, 50000);
	printf("-------");

	while(alive_processes > 0){
		pause();
	}

	print_result();

	return 0;
}


