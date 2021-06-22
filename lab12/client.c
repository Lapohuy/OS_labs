#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "strings.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

#define MAX_STRING_LENGTH 512

typedef struct file_receive
{
	char file_name[256];
	size_t file_size;
} file_receive_struct;

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

void send_client_data_to_server(const u_int16_t tcp_socket_fd, const char* client_data)
{
	uint32_t received_bytes = 0;

	received_bytes = write(tcp_socket_fd, client_data, MAX_STRING_LENGTH);
	if (received_bytes < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		exit(0);
	}
}

int main(int argv, char* argc[])
{
	if (argv <= 1)
	{
		printf("Error! Not enough arguments! Required (1): <ip-address>:<port>\n");
		return -1;
	}

	uint64_t received_bytes = 0;

	u_int16_t tcp_socket_fd = 0;

	char client_send_string[MAX_STRING_LENGTH];
	char server_recv_string[MAX_STRING_LENGTH];

	char* tcp_server_ip = parse_substring_from(argc[1], ':');
	char* tcp_server_port = parse_substring_from_to(argc[1], ':', '\0');

	struct sockaddr_in server_address;
	struct sockaddr_in client_address;

	bzero(client_send_string, sizeof(client_send_string));
	bzero(server_recv_string, sizeof(server_recv_string));

	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));

	if ((tcp_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror(NULL);
		return -1;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(tcp_server_port));

	if (inet_aton(tcp_server_ip, &server_address.sin_addr) == 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;
	}

	if (connect(tcp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;
	}

	while (1)
	{
		printf("[COMMAND]: ");
		
		fgets(client_send_string, MAX_STRING_LENGTH - 1, stdin);

		if (strncmp(client_send_string, "get", 3) == 0)
		{
			send_client_data_to_server(tcp_socket_fd, client_send_string);

			unsigned short count_files_to_receive = 0;

			received_bytes = read(tcp_socket_fd, &count_files_to_receive, sizeof(count_files_to_receive));
			if (received_bytes < 0)
			{
				perror(NULL);
				close(tcp_socket_fd);
				return -1;
			}

			count_files_to_receive--;

			printf("Receiving %u files from server...\n", count_files_to_receive);

			while (count_files_to_receive)
			{
				file_receive_struct file_to_receive;

				received_bytes = read(tcp_socket_fd, &file_to_receive, sizeof(file_to_receive));
				if (received_bytes < 0)
				{
					perror(NULL);
					close(tcp_socket_fd);
					return -1;
				}

				if (!file_to_receive.file_size)
				{
					printf("%s is not exist in the server\n", file_to_receive.file_name);
					count_files_to_receive--;
					continue;
				}
				
				FILE* file = fopen(file_to_receive.file_name, "w");
				file_to_receive.file_size = file_to_receive.file_size + 1;

				char file_buffer[file_to_receive.file_size];

				received_bytes = read(tcp_socket_fd, &file_buffer, file_to_receive.file_size);
				if (received_bytes < 0)
				{
					perror(NULL);
					close(tcp_socket_fd);
					return -1;
				}

				fprintf(file, "%s", file_buffer);
				fclose(file);

				printf("%s received %li\\%li bytes ::: DONE\n", file_to_receive.file_name, received_bytes, file_to_receive.file_size);

				count_files_to_receive--;
			}
		}
		else if (strncmp(client_send_string, "exit", 4) == 0)
		{
			printf("Shutting down...\n");
			close(tcp_socket_fd);
			break;
		}
		else if (strncmp(client_send_string, "date", 4) == 0)
		{
			send_client_data_to_server(tcp_socket_fd, client_send_string);

			received_bytes = read(tcp_socket_fd, server_recv_string, MAX_STRING_LENGTH);
			if (received_bytes < 0)
			{
				perror(NULL);
				close(tcp_socket_fd);
				return -1;
			}
			
			printf("%s", server_recv_string);
		}
		else if (strncmp(client_send_string, "files", 5) == 0)
		{
			send_client_data_to_server(tcp_socket_fd, client_send_string);

			received_bytes = read(tcp_socket_fd, server_recv_string, MAX_STRING_LENGTH);
			if (received_bytes < 0)
			{
				perror(NULL);
				close(tcp_socket_fd);
				return -1;
			}

			printf("%s", server_recv_string);						
		}
		else if (strncmp(client_send_string, "clear", 5) == 0)
			system("clear");
		else
			printf("Unknown command!\n");
	}

	free(tcp_server_ip);
	free(tcp_server_port);

	return 0;
}