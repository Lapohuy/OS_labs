#include "stdio.h"
#include "sys/shm.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"

#include "sys/types.h"
#include "sys/wait.h"
#include <cstdlib>
#include <stdlib.h>

void *allocateSharedMemory(size_t mem_size, int& mem_id)
{
	mem_id = shmget(IPC_PRIVATE, mem_size, 0600 | IPC_CREAT | IPC_EXCL);
	if (mem_id <= 0)
	{
		perror("error with memId");
		return NULL;
	}
	
	void *mem = shmat(mem_id, 0, 0);
	if (NULL == mem)
	{
		perror("error with shmat");
	}
	
	return mem;
}

void print_arr_int(int* arr_ptr)
{
	for (int i = 0; i < 20; i++)
		printf("%i ", *(arr_ptr + i));
	printf("\n");
}

int compare_int(const void* a, const void* b)
{
	return *((int*) a) - *((int*) b);
}

void child_main_code(int* shared_mem_ptr)
{
	qsort(shared_mem_ptr, 20, 4, compare_int);
	print_arr_int(shared_mem_ptr);
	exit(0);
}

int main(void)
{
	int mem_id;
	int* shared_mem_ptr = (int*) allocateSharedMemory(80, mem_id);
	printf("mem_id = %d\n", mem_id);
	
	srand(time(NULL));
	for (int i = 0; i < 20; i++)
	{
		*(shared_mem_ptr + i) = rand() % 100;
	}
	print_arr_int(shared_mem_ptr);
	
	pid_t child_process_id = fork();
	
	if (child_process_id == -1)
	{
		perror("error with fork() - process 1\n");
	}
	else if (child_process_id == 0)
	{
		child_main_code(shared_mem_ptr);
	}
	else
	{
		waitpid(child_process_id, NULL, 0);
	}
	
	char shared_mem_del_com[124];
	sprintf(shared_mem_del_com, "ipcrm -m %i", mem_id);
	system(shared_mem_del_com);
	
	return 0;
}