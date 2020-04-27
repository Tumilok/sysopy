#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>
#include "common.h"

int sem_id;

void error(char* msg)
{
	printf("%s Error: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

void sigint_handler(int signal)
{
	exit(EXIT_SUCCESS);
}

void terminate()
{
	printf("Terminating worker2 %d\n", getpid());
}

void prepare(int sem_id, int orders_id)
{
	struct sembuf sops[3];

	sops[0].sem_num = IN_USE;	//wait until nobody modifies orders
	sops[0].sem_op = 0;
	sops[0].sem_flg = 0;

	sops[1].sem_num = ARE_TO_PREP;	//wait until there are orders to prepare
	sops[1].sem_op = 0;
	sops[1].sem_flg = 0;

	sops[2].sem_num = IN_USE;	//mark orders as currently being used
	sops[2].sem_op = 1;
	sops[2].sem_flg = 0;

	if (semop(sem_id, sops, 3) == -1)
	{
		error("Could not execute operations on semaphores.");
	}

	orders* orders = shmat(orders_id, NULL, 0);

	orders -> vals[orders->first_to_prep] *= 2;
	int n = orders -> vals[orders -> first_to_prep];

	orders -> num_to_prep--;
	orders -> num_to_send++;
	orders -> first_to_prep = (orders->first_to_prep + 1) % MAX_ORDERS;

	printf("(%d %ld) Przygotowalem liczbe wielkosci %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
		   getpid(), time(NULL), n, orders->num_to_prep, orders->num_to_send);

	struct sembuf finalize[3];

	int semsop_idx = 0;

	if (orders -> num_to_prep == 0)
	{
		finalize[semsop_idx].sem_num = ARE_TO_PREP;		//there are no more to prepare
		finalize[semsop_idx].sem_op = 1;
		finalize[semsop_idx].sem_flg = 0;
		semsop_idx++;
	}

	if (semctl(sem_id, 2, GETVAL, NULL) == 1)		//if there were no more to send before
	{
		finalize[semsop_idx].sem_num = ARE_TO_SEND;
		finalize[semsop_idx].sem_op = -1;
		finalize[semsop_idx].sem_flg = 0;
		semsop_idx++;
	}

	finalize[semsop_idx].sem_num = IN_USE;
	finalize[semsop_idx].sem_op = -1;
	finalize[semsop_idx].sem_flg = 0;
	semsop_idx++;

	if (semop(sem_id, finalize, semsop_idx) == -1)
	{
		error("Could not execute operations on semaphores.");
	}
	shmdt(orders);
}

int main(int argc, char *argv[])
{
	if (atexit(terminate) == -1)
	{
		error("atexit failed.");
	}
	signal(SIGINT, sigint_handler);

	srand(time(NULL));

	char cwd[PATH_MAX];
	getcwd(cwd, sizeof cwd);

	key_t sem_key = ftok(cwd, SEM_ID);
	sem_id = semget(sem_key, 0, 0);
	if (sem_id == -1)
	{
		error("Could not access semaphores.");
	}

	key_t ord_key = ftok(cwd, ORD_ID);
	int orders_id = shmget(ord_key, 0, 0);
	if (orders_id == -1)
	{
		error("Could not access shared memory.");
	}

	while(1)
	{
		usleep((rand() % 5 + 1) * RAND_TIME_MUL);
		prepare(sem_id, orders_id);
	}
}