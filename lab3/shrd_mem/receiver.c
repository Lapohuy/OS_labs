#include "stdio.h"
#include "sys/shm.h"
#include "stdlib.h"

void print_array_int(int* arr)
{
	for (int i = 0; i < 20; i++)
		printf("%i ", *(arr + i));
	printf("\n");
}

int compare_int_value(const void* a, const void* b)
{
	return *((int*) a) - *((int*) b);
}

int main(int argv, char* argc[])
{
	if (argv <= 1)
	{
		printf("not enough params\n");
		return -1;
	}
	
	char* paramStr = argc[1];
	int memId = atoi(paramStr);
	
	if (memId == 0)
	{
		printf("incorrect parameter string: %s\n", paramStr);
		return -2;
	}
	
	printf("receiving the memory data: shmid = %i\n", memId);
	
	int* mem = (int*)shmat(memId, 0, 0);
	if (NULL == mem)
	{
		printf("error with shmat()\n");
		return -3;
	}
	
	printf("receiving next array:\n");
	print_array_int(mem);
	
	qsort(mem, 20, 4, compare_int_value);
	
	printf("sorted array:\n");
	print_array_int(mem);
	
	return 0;
}
