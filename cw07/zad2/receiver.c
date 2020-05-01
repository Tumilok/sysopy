#include "common.h"

sem_t* in_use_sem;
sem_t* are_to_prep_sem;
sem_t* are_to_send_sem;
sem_t* are_free_sem;

void error_exit(char* message){
	printf("%s Error: %s\n", message, strerror(errno));
	exit(EXIT_FAILURE);
}

void sigint_handler(int signo){
	exit(EXIT_SUCCESS);
}

void exit_fun(){
	sem_close(in_use_sem);
	sem_close(are_to_prep_sem);
	sem_close(are_to_send_sem);
	sem_close(are_free_sem);
}

int get_random_number(){
	return 1 + rand()%100;
}

sem_t* open_sem(char* name){
	sem_t* sem = sem_open(name, O_RDWR);
	if(sem == SEM_FAILED) error_exit("Could not create semaphore.");
	return sem;
}

void add(int orders_fd){

	int order_value = get_random_number();

	sem_wait(are_free_sem);

	if(sem_wait(in_use_sem) == -1) error_exit("Could not wait in_use_sem.");


	orders* orders =  mmap(NULL, sizeof(orders), PROT_READ | PROT_WRITE, MAP_SHARED, orders_fd, 0);

	sem_post(in_use_sem);
	orders->storage[orders->first_free] = order_value;
	orders->first_free = (orders->first_free + 1) % MAX_ORDERS_NUMBER;
	orders->num_to_prep++;

	printf("(%d %ld) Dostalem liczbe: %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
		   getpid(), time(NULL), order_value, orders->num_to_prep, orders->num_to_send);



	if(!(orders->first_free == orders->first_to_send || orders->first_free == orders->first_to_pack)){
		sem_post(are_free_sem);	//there are still free places
	}

	if(munmap(orders, sizeof(orders)) == -1) error_exit("Could not unmount shared memory.");


	int val;

	sem_getvalue(are_to_prep_sem, &val);
	if(val == 0){				//if there were no more to prepare before
		sem_post(are_to_prep_sem);
	}

	sem_post(in_use_sem);
}

int main(int argc, char** argv){
	signal(SIGINT, sigint_handler);
	if(atexit(exit_fun) == -1) error_exit("atexit failed.");

	srand(time(NULL));

	in_use_sem = open_sem(BUSY);

	are_to_prep_sem = open_sem(PACK);
	are_to_send_sem = open_sem(SEND);
	are_free_sem = open_sem(FREE);


	int orders_fd = shm_open(ORDER_NAME, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	if(orders_fd == -1) error_exit("Could not open shared memory.");


	while(1){
		usleep((rand()%5 + 1) * 100000);
		add(orders_fd);
	}
}