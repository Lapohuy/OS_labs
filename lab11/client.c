include "time.h"
#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "strings.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include <time.h>

char* parse_substring_from(char* src_string, const char match_char)
{
	char* first_match = strchr(src_string, match_char);

	size_t bytes_for_malloc = first_match - src_string;
	char* str_mem = malloc(bytes_for_malloc + 1);
	memcpy(str_mem, src_string, bytes_for_malloc);

	str_mem[bytes_for_malloc] = '\0';

	return str_mem;
}

char* parse_substring_from_to(char* src_string, const char match_char_from, const char match_char_to)
{
	char* first_match_from = strchr(src_string, match_char_from) + 1;
	char* first_match_to = strchr(src_string, match_char_to);

	size_t bytes_for_malloc = first_match_to - first_match_from;
	char* str_mem = malloc(bytes_for_malloc + 1);
	memcpy(str_mem, first_match_from, bytes_for_malloc);

	str_mem[bytes_for_malloc] = '\0';

	return str_mem;
}

void array_fill_random_value(int32_t* array, uint16_t size, int32_t min, int32_t max)
{
	for (size_t i = 0; i < size; i++)
		array[i] = min + rand() % max;
}

void print_array_values(int32_t* array, uint16_t size)
{	
	for (size_t i = 0; i < size; i++)
		printf("%i ", array[i]);
	printf("\n");
}

int main(int argv, char* argc[])
{
	if (argv <= 4)
	{
		printf("Error! Not enough arguments! Required (4): <ip-address>:<port>, array size, array min, array max\n");
		return -1;
	}

	srand(time(NULL));

	u_int16_t udp_socket_fd = 0;

	char* udp_server_ip = parse_substring_from(argc[1], ':');
	char* udp_server_port = parse_substring_from_to(argc[1], ':', '\0');

	struct sockaddr_in server_address;
	struct sockaddr_in client_address;

	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));

	// Try to create UDP socket
	if ((udp_socket_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror(NULL);
		return -1;
	}

	client_address.sin_family = AF_INET;
	client_address.sin_port = htons(0);
	client_address.sin_addr.s_addr = htons(INADDR_ANY);

	if (bind(udp_socket_fd, (struct sockaddr*) &client_address, sizeof(client_address)) < 0)
	{
		perror(NULL);
		close(udp_socket_fd);
		return -1;
	}

	printf("Try to connect to server with address: %s:%s\n", udp_server_ip, udp_server_port);

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(udp_server_port));
	
	if (inet_aton(udp_server_ip, &server_address.sin_addr) == 0)
	{
		printf("Error! Invalid IP address!\n");
		close(udp_socket_fd);
		return -1;
	}

	unsigned short client_array_size = atoi(argc[2]);
	int* client_array = malloc(client_array_size * sizeof(int));

	array_fill_random_value(client_array, client_array_size, atoi(argc[3]), atoi(argc[4]));

	printf("==== Generate next array ====\n");
	print_array_values(client_array, client_array_size);
	printf("=============================\n");

	// Send client array size to server
	if (sendto(udp_socket_fd, &client_array_size, sizeof(client_array_size), 0, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
	{
		perror(NULL);
		close(udp_socket_fd);
		return -1;
	}

	// Send client array to server
	clock_t start_time = clock();
	if (sendto(udp_socket_fd, client_array, sizeof(int) * client_array_size, 0, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
	{
		perror(NULL);
		close(udp_socket_fd);
		return -1;
	}

	// Receive sorted array from server
	int received_bytes_from_server = 0;

	received_bytes_from_server = recvfrom(udp_socket_fd, client_array, sizeof(int) * client_array_size, 0, (struct sockaddr*) NULL, NULL);
	clock_t end_time = clock();

	if (received_bytes_from_server < 0)
	{
		perror(NULL);
		close(udp_socket_fd);
		return -1;
	}

	printf("==== Response from server ====\n");
	print_array_values(client_array, client_array_size);
	printf("Time required: %f\n", (end_time - start_time) / (double) CLOCKS_PER_SEC);
	printf("==============================\n");

	close(udp_socket_fd);

	free(client_array);

	free(udp_server_ip);
	free(udp_server_port);

	return 0;
}