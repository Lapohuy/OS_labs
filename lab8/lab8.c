#include "stdio.h"
#include "fcntl.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include "sys/types.h"

int main(int argv, char* argc[])
{
	if (argv <= 2)
	{
		printf("Error! Not enough arguments! Required (2): input file, output file\n");
		return -1;
	}

	const char* input_file_name = argc[1];
	const char* output_file_name = argc[2];

	int input_file = open(input_file_name, O_RDONLY, 0600);

	if (input_file < 0)
	{
		perror("Error! Cant open input file!\n");
		return -1;
	}

	struct stat input_file_stat;
	if (fstat(input_file, &input_file_stat) == -1)
	{
		perror("Error! Cant get input file stat!\n");
		return -1;
	}

	int output_file = open(output_file_name, O_RDWR | O_CREAT, 0600);
	if (output_file < 0)
	{
		perror("Error! Cant get output file disc!\n");
		return -1;
	}

	int file_length = input_file_stat.st_size;
	ftruncate(output_file, file_length);

	char* output_file_ptr = (char*) mmap(NULL, file_length, PROT_WRITE | PROT_READ, MAP_SHARED, output_file, 0);

	if (output_file_ptr == MAP_FAILED)
	{
		perror("Output file mapping failed!\n");
		return -1;
	}

	ssize_t input_file_read_bytes = read(input_file, output_file_ptr, file_length);

	printf("Need to write: %i\n", file_length);
	printf("Write in file: %i\n", (int) input_file_read_bytes);

	close(input_file);
	close(output_file);

	munmap((void*) output_file_ptr, file_length);

	return 0;
}