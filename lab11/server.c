#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdint.h"
#include "strings.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "sys/socket.h"

int compare_int_value(const void* a, const void* b)
{
	return *((int*) b) - *((int*) a);
}

void print_server_port(const u_int16_t tcp_socket_fd)
{
	struct sockaddr_in socket_address;
	socklen_t socket_len = sizeof(socket_address);

	getsockname(tcp_socket_fd, (struct sockaddr*) &socket_address, &socket_len);
	printf("Server started in %i port\n", ntohs(socket_address.sin_port));
}

int main(int argv, char* argc[])
{
	if (argv <= 1)
	{
		printf("Error! Not enough arguments! Required (1): port\n");
		return -1;
	}

	uint32_t client_length = 0;

	u_int16_t udp_socket_fd = 0;
	u_int16_t host_port = atoi(argc[1]);

	struct sockaddr_in server_address;
	struct sockaddr_in client_address;

	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(host_port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Try to create UDP socket
	if ((udp_socket_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror(NULL);
		return -1;
	}

	// Try to set up UDP socket
	if (bind(udp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
	{
		printf("Error! Cant get %i port! Trying to get another!\n", host_port);

		server_address.sin_port = htons(0);
		if (bind(udp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
		{
			perror(NULL);
			close(udp_socket_fd);
			return -1;
		}
	}

	print_server_port(udp_socket_fd);

	while (1)
	{
		int client_received_bytes = 0;		
		
		client_length = sizeof(client_length);

		// Wait to get array size from client
		unsigned short client_array_size = 0;
		client_received_bytes = recvfrom(udp_socket_fd, &client_array_size, sizeof(client_array_size), 0, (struct sockaddr*) &client_address, &client_length);
	
		if (client_received_bytes < 0)
		{
			perror(NULL);
			close(udp_socket_fd);
			return -1;
		}

		printf("============ Incoming form ============\n");
		printf("Received next array size: %i\n", client_array_size);
		printf("Wait to receive array...\n");

		// Wait to get array with "client_array_size" size
		int* client_array = malloc(client_array_size * sizeof(int));
		client_received_bytes = recvfrom(udp_socket_fd, client_array, sizeof(int) * client_array_size, 0, (struct sockaddr*) &client_address, &client_length);
	
		if (client_received_bytes < 0)
		{
			perror(NULL);
			close(udp_socket_fd);
			return -1;
		}

		printf("Client array is received\n");
		printf("Sorting...\n");

		// Sort received array and send it back to client
		qsort(client_array, client_array_size, sizeof(int), compare_int_value);
		
		printf("Sending back...\n");

		if (sendto(udp_socket_fd, client_array, sizeof(int) * client_array_size, 0, (struct sockaddr*) &client_address, client_length) < 0)
		{
			perror(NULL);
			close(udp_socket_fd);
			return -1;
		}

		free(client_array);

		printf("========================================\n");
	}

	return 0;
}