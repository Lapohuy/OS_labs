#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "sys/wait.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/cdefs.h>

void print_arr_int(int* arr, int size)
{
   	for (int i = 0; i < size; i++)
		printf("%d ", arr[i]);
	printf("\n");
}

int compare_int_value(const void* a, const void* b)
{
	return *((int*) b) - *((int*) a);
}

int main(int argv, char* argc[])
{
	if (argv <= 1)
	{
		printf("Error! Not enough params!\n");
		return -1;
	}

	int arr_size = atoi(argc[1]);
	int* array = malloc(sizeof(int) * arr_size);

	srand(time(NULL));
	for (int i = 0; i < arr_size; i++)
	{
		array[i] = rand() % 100;
		printf("%d ", array[i]);
	}
	printf("\n");

	int FDpipe[2], FDfifo;
	size_t size;

	if (pipe(FDpipe) < 0)
	{
		printf("Error with creating the pipe!\n");
		return -1;
	}

	const char* file_name = "tmp.fifo";
	(void) umask(0);

	if (mknod(file_name, S_IFIFO | 0666, 0) < 0)
	{
		printf("Error with creating the FIFO!\n");
		return -1;
	}

	pid_t child_process = fork();

	if (child_process == -1)
	{
		printf("Error with forking the child!\n");
		return -1;
	}
	else if (child_process == 1)
	{
		close(FDpipe[0]);
		size = write(FDpipe[1], array, sizeof(int) * arr_size);

		if (size != sizeof(int) * arr_size)
		{
			printf("Error! Parent can't write all array!\n");
			return -1;
		}

		close(FDpipe[1]);

		if ((FDfifo = open(file_name, O_RDONLY)) < 0)
		{
			printf("Error! Parent can't open FIFO for reading!\n");
			return -1;
		}

		size = read(FDfifo, array, sizeof(int) * arr_size);
		if (size < 0)
		{
			printf("Error! Parent can't read array using FIFO!\n");
			return -1;
		}

		printf("Parent received this array: ");
		print_arr_int(array, arr_size);
		
		close(FDfifo);
	}
	else if (child_process == 0)
	{
		close(FDpipe[1]);

		int* new_array = malloc(sizeof(int) * arr_size);
		size = read(FDpipe[0], new_array, sizeof(int) * arr_size);

		if (size < 0)
		{
			printf("Error! Child can't read array!\n");
			return -1;
		}

		qsort(new_array, arr_size, sizeof(int), compare_int_value);
		close(FDpipe[0]);

		if ((FDfifo = open(file_name, O_WRONLY)) < 0)
		{
			printf("Error! Child can't open FIFO for writing!\n");
			return -1;
		}

		size = write(FDfifo, new_array, sizeof(int) * arr_size);
		if (size != sizeof(int) * arr_size)
		{
			printf("Error! Child can't write all array using FIFO!\n");
			return -1;
		}

		close(FDfifo);
		free(new_array);

		printf("Child process done!\n");
		return 0;
	}

	char delete_fifo_file[124];
	sprintf(delete_fifo_file, "rm %s", file_name);
	system(delete_fifo_file);

	free(array);
	return 0;
}