#include "time.h"
#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "dirent.h"
#include "strings.h"
#include "sys/stat.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

#define MAX_STRING_LENGTH 512
#define SERVER_FOLDER "server_folder/"

typedef struct file_receive
{
	char file_name[256];
	size_t file_size;
} file_receive_struct;

void client_service(const u_int16_t tcp_socket_fd, const struct sockaddr_in* server_address)
{
	uint32_t received_bytes = 0;
	char client_command[MAX_STRING_LENGTH];

	while (1)
	{
		// There process working with current client
		received_bytes = read(tcp_socket_fd, client_command, MAX_STRING_LENGTH - 1);
		
		if (received_bytes < 0)
		{
			perror(NULL);
			break;
		}

		if (received_bytes == 0)
		{
			printf("[SERVER][INFO]: One of clients break the connection with server\n");
			break;
		}
		
		if (strncmp(client_command, "get", 3) == 0)
		{
			unsigned short files_count = 0;

			char* token = strtok(client_command, " ");
			char* file_names_array[128];
			
			// Parsing file names from client command
			while (token)
			{
				unsigned short file_name_length = strlen(token) + 1;
				file_names_array[files_count] = malloc(file_name_length);

				strcpy(file_names_array[files_count], token);
				file_names_array[files_count][file_name_length - 1] = '\0';

				files_count++;
				token = strtok(NULL, " ");
			}

			unsigned last_char = strlen(file_names_array[files_count - 1]) - 1;
			file_names_array[files_count - 1][last_char] = '\0';

			received_bytes = write(tcp_socket_fd, &files_count, sizeof(files_count));
			if (received_bytes < 0)
			{
				printf("[SERVER][ERROR]: Cant send information to one of clients\n");
				break;
			}

			for (int i = 1; i < files_count; i++)
			{
				char file_path[MAX_STRING_LENGTH] = SERVER_FOLDER;
				strcat(file_path, file_names_array[i]);
				
				FILE* file = fopen(file_path, "r");
				file_receive_struct file_to_send;

				// Get a file name
				strcpy(file_to_send.file_name, file_names_array[i]);

				if (!file)
				{
					file_to_send.file_size = 0;

					received_bytes = write(tcp_socket_fd, &file_to_send, sizeof(file_to_send));
					if (received_bytes < 0)
					{
						printf("[SERVER][ERROR]: Cant send information to one of clients\n");
						break;
					}
					
					continue;
				}

				// Get a file size
				fseek(file, 0L, SEEK_END);
				file_to_send.file_size = ftell(file);
				fseek(file, 0L, SEEK_SET);

				// Send to client the file information
				received_bytes = write(tcp_socket_fd, &file_to_send, sizeof(file_to_send));
				if (received_bytes < 0)
				{
					printf("[SERVER][ERROR]: Cant send information to one of clients\n");
					break;
				}

				char file_buffer[file_to_send.file_size + 1];
				fread(file_buffer, sizeof(char), file_to_send.file_size, file);
				file_buffer[file_to_send.file_size] = '\0';

				fclose(file);

				received_bytes = write(tcp_socket_fd, file_buffer, file_to_send.file_size + 1);
				if (received_bytes < 0)
				{
					printf("[SERVER][ERROR]: Cant send information to one of clients\n");
					break;
				}
			}

			for (int i = 0; i < files_count; i++)
				free(file_names_array[i]);
		}
		else if (strncmp(client_command, "date", 4) == 0)
		{
			time_t server_time;
			struct tm* time_info;

			time(&server_time);
			time_info = localtime(&server_time);

			char* asc_time = asctime(time_info);

			received_bytes = write(tcp_socket_fd, asc_time, strlen(asc_time) + 1);
			if (received_bytes < 0)
			{
				printf("[SERVER][ERROR]: Cant send information to one of clients\n");
				break;
			}
		}
		else if (strncmp(client_command, "files", 4) == 0)
		{
			char server_answer[MAX_STRING_LENGTH];

			memset(server_answer, 0, sizeof(server_answer));
			server_answer[MAX_STRING_LENGTH - 1] = '\0';

			DIR* directory = opendir(SERVER_FOLDER);
			
			if (!directory)
			{
				strcat(server_answer, "Server cant open directory\n");
				received_bytes = write(tcp_socket_fd, server_answer, MAX_STRING_LENGTH);
				if (received_bytes < 0)
				{
					printf("[SERVER][ERROR]: Cant send information to one of clients\n");
					break;
				}
				continue;
			}

			struct dirent* file;
			struct stat file_stat;

			while((file = readdir(directory)) != NULL)
			{
				if (strncmp(file->d_name, "..", 2) == 0 || strncmp(file->d_name, ".", 1) == 0)
					continue;
				
				strcat(server_answer, file->d_name);
				strcat(server_answer, "\n");
			}

			closedir(directory);

			received_bytes = write(tcp_socket_fd, server_answer, MAX_STRING_LENGTH);
			if (received_bytes < 0)
			{
				printf("[SERVER][ERROR]: Cant send information to one of clients\n");
				break;
			}
		}
	}

	close(tcp_socket_fd);
	exit(0);
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

	u_int16_t tcp_socket_fd;
	u_int16_t tcp_newsocket_fd;

	struct sockaddr_in server_address;
	struct sockaddr_in client_address;

	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));

	// Try to create TCP socket
	if ((tcp_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror(NULL);
		return -1;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(argc[1]));
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	// Try to set up TCP socket
	if (bind(tcp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
	{
		printf("Error! Cant get %s port! Trying to get another!\n", argc[1]);

		server_address.sin_port = htons(0);
		if (bind(tcp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
		{
			perror(NULL);
			close(tcp_socket_fd);
			return -1;
		}
	}

	print_server_port(tcp_socket_fd);

	if (listen(tcp_socket_fd, 5) < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;
	}

	pid_t child_process_server = fork();

	if (child_process_server == -1)
	{
		printf("[ROOT][CRITICAL]: Server cannot be started\n");
		return -1;
	}
	else if (child_process_server == 0)
	{
		while (1)
		{
			client_length = sizeof(client_address);

			tcp_newsocket_fd = accept(tcp_socket_fd, (struct sockaddr*) &client_address, &client_length);
			if (tcp_newsocket_fd < 0)
			{
				perror(NULL);
				close(tcp_socket_fd);
				return -1;
			}

			pid_t child_process_pid = fork();

			if (child_process_pid == -1)
				printf("[SERVER][ERROR]: Cant fork new process!\n");
			else if (child_process_pid == 0)
				client_service(tcp_newsocket_fd, &server_address);
			else
				printf("[SERVER][INFO]: New process has been started. Continue listening...\n");
		}
	}
	else
	{
		printf("[ROOT][INFO]: Server has been started\n");

		while (1)
		{
			// This is root process. It processes commands for the server
			char server_command[512];

			fgets(server_command, MAX_STRING_LENGTH - 1, stdin);

			if (strncmp(server_command, "exit", 4) == 0)
			{
				printf("[SERVER][INFO]: Server shutdown...\n");
				system("killall ./server");
				return 0;
			}
			else if (strncmp(server_command, "date", 4) == 0)
				system("date");
			else if (strncmp(server_command, "files", 5) == 0)
				system("ls -l server_folder/");
			else if (strncmp(server_command, "clear", 5) == 0)
				system("clear");
			else
				printf("[SERVER][COMMAND]: Unknown command!\n");
		}

	}

	return 0;
}