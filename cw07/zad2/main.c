#include "common.h"

const int workers_num = RECEIVER_NUM + PACKER_NUM + SENDER_NUM;

pid_t worker_pids[RECEIVER_NUM + PACKER_NUM + SENDER_NUM];
int sem_id;
int orders_id;

void error_exit(char* message){
	printf("%s Error: %s\n", message, strerror(errno));
	exit(EXIT_FAILURE);
}

void sigint_handle(int signo){
	exit(EXIT_SUCCESS);

}

void exit_function(){

	for(int i = 0; i<workers_num; i++){
		kill(worker_pids[i], SIGINT);
	}

	sem_unlink(IN_USE);
	sem_unlink(ARE_TO_PREP);
	sem_unlink(ARE_TO_SEND);
	sem_unlink(ARE_FREE);

	if(shm_unlink(ORD_NAME) == -1) error_exit("Could not delete shared memory.");

}

sem_t* create_sem(char* name, int value){
	int oflag = O_RDWR | O_CREAT;

	sem_t* sem = sem_open(name, oflag, S_IRWXU | S_IRWXG | S_IRWXO, value);
	if(sem == SEM_FAILED) error_exit("Could not create semaphore.");

	return sem;
}

void create_sems(){
	sem_t* in_use_sem = create_sem(IN_USE, 1);
	sem_t* are_to_prep_sem = create_sem(ARE_TO_PREP, 0);
	sem_t* are_to_send_sem = create_sem(ARE_TO_SEND, 0);
	sem_t* are_free_sem = create_sem(ARE_FREE, 1);

	sem_close(in_use_sem);
	sem_close(are_to_prep_sem);
	sem_close(are_to_send_sem);
	sem_close(are_free_sem);
}

void create_orders(){
	int orders_fd;
	orders_fd = shm_open(ORD_NAME, O_RDWR | O_CREAT, 0666);
	if(orders_fd == -1) error_exit("Could not create shared memory.");

	if(ftruncate(orders_fd, sizeof(orders)) == -1) error_exit("Could not define size of shared memory.");

	orders* orders = mmap(NULL, sizeof(orders), PROT_READ | PROT_WRITE, MAP_SHARED, orders_fd, 0);

	orders->num_to_prep = orders->num_to_send = 0;
	orders->first_to_prep = orders->first_to_send = 0;
	orders->first_free = 0;

	for(int i = 0; i < MAX_ORDERS; i++){
		orders->vals[i] = -1;
	}

	if(munmap(orders, sizeof(orders)) == -1) error_exit("Could not unmount shared memory.");

}

int main(int argc, char** argv){

	atexit(exit_function);
	signal(SIGINT, sigint_handle);

	create_sems();

	create_orders();

	for(int i = 0; i < workers_num; i++){

		pid_t worker_pid = fork();

		if(worker_pid == 0) {
			if(i < RECEIVER_NUM) execlp("./receiver", "receiver", NULL);
			else if (i < RECEIVER_NUM + PACKER_NUM) execlp("./packer", "packer", NULL);
			else execlp("./sender", "sender", NULL);
		}

		worker_pids[i] = worker_pid;

	}

	for(int i = 0; i < workers_num; i++){
		wait(NULL);
	}
}