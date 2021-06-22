#include "stdlib.h"
#include "stdio.h"

int main(int argv, char* argc[])
{
	if (argv <= 2)
	{
		printf("Error! Not enough arguments! Required (2): file name, count (in bytes)\n");
		return -1;
	}

	const char* file_name = argc[1];
	size_t bytes_to_read = atoi(argc[2]);

	FILE* file_in = fopen(file_name, "r");

	if (!file_in)
	{
		printf("Error! Cant open this file!\n");
		return -1;
	}

	char* buffer = malloc(bytes_to_read);
	int result = fread(buffer, sizeof(char), bytes_to_read, file_in);

	if (!result)
		printf("Cant read this file!\n");
	else
	{
		for (size_t i = 0; i < bytes_to_read; i++)
			printf("int = %i \t char = %c \t (0x%x)\n", buffer[i], buffer[i], buffer[i]);
		printf("\n");
	}

	free(buffer);
	fclose(file_in);

	return 0;
}