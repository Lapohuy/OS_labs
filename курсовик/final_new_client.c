#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include <strings.h>

#include "utility.h"

void print_programm_use_and_exit()
{
	printf("Use: ./client <args>\n");
	printf("Game mods options:\n");
	printf("<round num> -- Player vs Computer\n");
	printf("<round num> <ip-address> <port> -- Player vs Player\n\n");
    printf("The correct number of rounds is from 1 to 1000\n");
	exit(1);
}

void player_versus_player(char* server_ip, int server_port, int round_num)
{
	//sockets setup
	
	u_int16_t tcp_socket_fd = 0;
	
	client_states state;
	
	socket_address_in server_address;
	socket_address_in client_address;

	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));

	if ((tcp_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror(NULL);
		exit(1);
	}
	
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);
	
	if (inet_aton(server_ip, &server_address.sin_addr) == 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

	if (connect(tcp_socket_fd, (socket_address*) &server_address, sizeof(server_address)) < 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

    if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

    if (state == CLIENT_STATE_WAITING_FOR_ANOTHER_PLAYER)
	{
		printf("You are successfully connected!\n");
		printf("Waiting for another player...\n");
	}

    if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

    if (state == CLIENT_STATE_WAITING_FOR_GAME_START)
    {
        printf("Both players are successfully connected!\n");
        printf("Waiting for game start...\n");
    }
    
    //round num check

    if (round_num < 1 || round_num > 1000)
    {
        printf("Wrong round number!\n");
        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);
    }

    if (write(tcp_socket_fd, &round_num, sizeof(int)) <= 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);
    
    server_answer serv_answ;

    if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

    if (serv_answ != SERVER_OK)
    {
        printf("Wrong round number!\n");
        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);
    }

    if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

    if (serv_answ != SERVER_OK)
    {
        printf("Check your round number and restart the game!\n");
        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);
    }

    //initialization section

    if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

    if (state == CLIENT_STATE_WAIT_TURN)
	{
		printf("You are second player!\n");
		printf("Waiting for another player...\n");
	}
	else
		printf("You are starting player!\n");

    //cycle of rounds

    finger finger_num;
	//int p_score, score;
	int step = 0;

    while (step < round_num)
	{
        if (state == CLIENT_STATE_MAKE_TURN)
		{
			printf("Please, write the number of fingers between 1 and 3 including!\n");
			scanf("%i ", (int*) &finger_num);
			printf("\n");

            //need to upgrade on inf entering

            if (write(tcp_socket_fd, &finger_num, sizeof(int)) <= 0)
		        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);
            
            if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
		        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

            if (serv_answ != SERVER_OK)
            {
                printf("Wrong input\n");
                throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);
            }

            //need to upgrade on inf entering

            if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

            if (state == CLIENT_STATE_WAIT_TURN)
		        printf("Waiting for another player...\n");
	        else
		        printf("Waiting for master...\n");
        }
        else if (state == CLIENT_STATE_WAIT_TURN)
        {
            if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

            if (state == CLIENT_STATE_MAKE_TURN)
		        printf("Its your turn!\n");
	        else
		        printf("Waiting for master...\n");
        }
        else
        {
            if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

            if (state == CLIENT_STATE_ROUND_WIN)
                printf("You won the round %i!\n", step + 1);
            else
                printf("You lost the round %i!\n", step + 1);

            if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		        throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

            if (state == CLIENT_STATE_GAME_WIN)
            {
                 printf("You won the game!\n");
                 break;
            }
            else if (state == CLIENT_STATE_GAME_LOSS)
            {
                 printf("You lost the game!\n");
                 break;
            }
            else if (state == CLIENT_STATE_MAKE_TURN)
                printf("Its your turn!\n");
	        else
		        printf("Waiting for master...\n");

            step++;
        }
    }

    //socket close section

    close(tcp_socket_fd);
	
	exit(0);
}

int main(int argc, char* argv[])
{
    char ip_index_from_argv, port_index_from_argv;
	int round_num_index_from_argv;

    //need to rewrite when pve mod will be done!!!!
	
	if (argc == 2)
	{
		//game vs pc
	}
	else if (argc == 4)
	{
		round_num_index_from_argv = 1;
		ip_index_from_argv = 2;
		port_index_from_argv = 3;
		
		player_versus_player(argv[ip_index_from_argv], atoi(argv[port_index_from_argv]), atoi(argv[round_num_index_from_argv]));	
	}
	else
		print_programm_use_and_exit();		
	
	return 0;
}