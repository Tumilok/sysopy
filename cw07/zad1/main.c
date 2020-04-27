#include "common.h"

const int workers_num = WORKER1_NUM + WORKER2_NUM + WORKER3_NUM;

pid_t worker_pids[WORKER1_NUM + WORKER2_NUM + WORKER3_NUM];

void terminate()
{
	for (int i = 0; i < workers_num; i++)
	{
		kill(worker_pids[i], SIGINT);
	}
	semctl(sem_id, 0, IPC_RMID, NULL);
	shmctl(orders_id, IPC_RMID, NULL);
}

int create_semaphores()
{
	key_t sem_key = get_sem_key();
	int sem_id;
	if ((sem_id = semget(sem_key, SEM_NUMBER, IPC_CREAT | 0666)) == -1)
	{
		printf("Couldn't create a set of semaphores.\n");
		exit(EXIT_FAILURE);
	}

	union semun arg_av;
	arg_av.val = 0;

	union semun arg_dis;
	arg_dis.val = 1;

	semctl(sem_id, IN_USE, SETVAL, arg_av);
	semctl(sem_id, ARE_FREE, SETVAL, arg_av);
	semctl(sem_id, ARE_TO_SEND, SETVAL, arg_dis);
	semctl(sem_id, ARE_TO_PREP, SETVAL, arg_dis);

	return sem_id;
}

int create_orders()
{
	key_t ord_key = get_ord_key();
	int orders_id;
	if ((orders_id = shmget(ord_key, (sizeof(orders)), IPC_CREAT | 0666)) == -1)
	{
		error("Could not create shared memory.");
	}
	orders* orders = shmat(orders_id, NULL, 0);

	orders -> num_to_prep = orders -> num_to_send = 0;
	orders -> first_to_prep = orders -> first_to_send = 0;
	orders -> first_free = 0;

	for (int i = 0; i < MAX_ORDERS; i++)
	{
		orders -> vals[i] = -1;
	}
	shmdt(orders);

	return orders_id;
}

void remove_shared_mem(int id)
{
	shmctl(id, IPC_RMID, 0);
}

int main(int argc, char *argv[])
{
	atexit(terminate);
	signal(SIGINT, sigint_handler);


	sem_id = create_semaphores();
	orders_id = create_orders();

	for (int i = 0; i < workers_num; i++)
	{
		if ((worker_pids[i] = fork()) == 0)
		{
			if (i < WORKER1_NUM)
			{
				execlp("./worker1", "worker1", NULL);
			}
			else if (i < WORKER1_NUM + WORKER2_NUM)
			{
				execlp("./worker2", "worker2", NULL);
			}
			else
			{
				execlp("./worker3", "worker3", NULL);
			}
		}
	}

	for (int i = 0; i < WORKER1_NUM; i++)
	{
		wait(NULL);
	}
	remove_shared_mem(orders_id);
}