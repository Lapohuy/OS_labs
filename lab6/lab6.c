#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "stdlib.h"
#include "sys/shm.h"
#include "sys/types.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "sys/wait.h"
#include <sys/ipc.h>

#define UNLOCK 1
#define LOCK  -1

void arr_rand_fill(int* arr, int size, int min, int max)
{
	for (int i = 0; i < size; i++)
		arr[i] = min + rand() % max;
}

void* alloc_shrd_mem(size_t mem_size, int* mem_id)
{
	*mem_id = shmget(IPC_PRIVATE, mem_size, 0600 | IPC_CREAT | IPC_EXCL);
	if (*mem_id <= 0)
	{
		perror("error with memId");
		return NULL;
	}

	void* mem = shmat(*mem_id, 0, 0);
	if (NULL == mem)
		perror("error with shmat");
	
	return mem;
}

void set_state(int sem_id, int num, int state)
{
	struct sembuf op;
	op.sem_op = state;
	op.sem_flg = 0;
	op.sem_num = num;
	semop(sem_id, &op, 1);
}

char set_state_nowait(int sem_id, int num, int state)
{
	struct sembuf op;
	op.sem_op = state;
	op.sem_flg = IPC_NOWAIT;
	op.sem_num = num;
	return semop(sem_id, &op, 1);
}

void swap_vals(int* first, int* second)
{
	int temp = *first;
	*first = *second;
	*second = temp;
}

void parent_code(int* arr, int arr_size, int sem_id, pid_t child_id)
{
	int count = 0;
	while (!waitpid(child_id, NULL, WNOHANG))
	{
		printf("Count %i\n", count);
		for (int i = 0; i < arr_size; i++)
		{
			if (set_state_nowait(sem_id, i, LOCK) == -1)
				printf("block ");
			else
			{
				printf("%d ", arr[i]);
				set_state(sem_id, i, UNLOCK);
			}
		}
		printf("\n");
		count++;
	}

	printf("Iteration is: %i\n", count);
}

void child_code(int* arr, int arr_size, int sem_id)
{
	double factor = 1.2473309;
	int step = arr_size - 1;

	while (step >= 1)
	{
		for (int i = 0; i + step < arr_size; i++)
		{
			set_state(sem_id, i, LOCK);
			set_state(sem_id, i + step, LOCK);

			if (arr[i] > arr[i + step])
				swap_vals(&arr[i], &arr[i + step]);

			set_state(sem_id, i + step, UNLOCK);
			set_state(sem_id, i, UNLOCK);
		}
		step /= factor;
	}

	exit(0);
}

void free_shrd_memory(int* mem_id);

void free_sems(int* sem_id);

int main(int argv, char* argc[])
{
	if (argv <= 3)
	{
		printf("Error! Not enough arguments! Required: 3 (arr_size, min, max)\n");
		return -1;
	}
	
	int arr_size = atoi(argc[1]);
	int arr_min = atoi(argc[2]);
	int arr_max = atoi(argc[3]);

	int mem_id;
	int* arr_ptr = alloc_shrd_mem(sizeof(int) * arr_size, &mem_id);

	arr_rand_fill(arr_ptr, arr_size, arr_min, arr_max);

	int sem_id = semget(IPC_PRIVATE, arr_size, 0600 | IPC_CREAT);

	if (sem_id < 0)
	{
		perror("Error with semget()!\n");
		return -1;
	}

	printf("Semaphore set id = %i\n", sem_id);

	for (int i = 0; i < arr_size; i++)
		set_state(sem_id, i, UNLOCK);

	pid_t child_process_id = fork();

	if (child_process_id == -1)
		perror("Error with fork() - process 1\n");
	else if (child_process_id == 0)
		child_code(arr_ptr, arr_size, sem_id);
	else
		parent_code(arr_ptr, arr_size, sem_id, child_process_id);

	free_sems(&sem_id);
	free_shrd_mem(&mem_id);

	return 0;
}

void free_shrd_mem(int* mem_id)
{
	char del_cmd_temp[124];
	sprintf(del_cmd_temp, "ipcrm -m %i", *mem_id);
	system(del_cmd_temp);
	*mem_id = 0;
}

void free_sems(int* sem_id)
{
	char del_cmd_temp[124];
	sprintf(del_cmd_temp, "ipcrm -s %i", *sem_id);
	system(del_cmd_temp);
	*sem_id = 0;
}