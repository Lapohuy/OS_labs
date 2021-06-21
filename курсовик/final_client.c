#include "time.h"
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
	exit(1);
}

finger finger_input_while(finger finger_num)
{
	finger finger_temp = finger_num;
	
	if (!is_player_input_correct(finger_temp))
    {
		while (!is_player_input_correct(finger_temp))
		{
			printf("Wrong input! Please, write the number of fingers between 1 and 3 including!\n");
			scanf("%i", (int*) &finger_temp);
			printf("\n");
			
			if (!is_player_input_correct(finger_temp))
				printf("Incorrect input, please, try again\n");
		}
	}
	
	printf("Your input is correct!\n");
	
	return finger_temp;
}

int round_num_input_while(int round_num)
{
	int round_num_temp = round_num;
	
	if (!is_round_num_correct(round_num_temp))
	{
		while (!is_round_num_correct(round_num_temp))
		{
			printf("Wrong input! Please, write the number of rounds between 1 and 1000 including!\n");
			scanf("%i", &round_num_temp);
			printf("\n");
			
			if(!is_round_num_correct(round_num_temp))
				printf("Incorrect input, please, try again\n");				
		}
	}
	
	printf("Your input is correct!\n");
	
	return round_num_temp;
}

int agreement_input_while(int agreement_input)
{
	int agreement_input_temp = agreement_input;

	if (!is_agreement_input_correct(agreement_input_temp))
	{
		while (!is_agreement_input_correct(agreement_input_temp))
		{
			printf("Wrong input! Please, enter correct answer - 1 or 0\n");
			scanf("%i", &agreement_input_temp);
			printf("\n");
					
			if (!is_agreement_input_correct(agreement_input_temp))
				printf("Incorrect input, please, try again\n");
		}
	}
	
	printf("Your input is correct!\n");
	
	return agreement_input_temp;
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
		throw_exception_and_exit(tcp_socket_fd, 4);

	if (connect(tcp_socket_fd, (socket_address*) &server_address, sizeof(server_address)) < 0)
		throw_exception_and_exit(tcp_socket_fd, 4);
	
	if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exception_and_exit(tcp_socket_fd, 4);
	
	if (state == CLIENT_STATE_WAITING_FOR_ANOTHER_PLAYER)
	{
		printf("You are successfully connected!\n");
		printf("Waiting for another player...\n");
	}
	
	if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exception_and_exit(tcp_socket_fd, 4);
	
	if (state == CLIENT_STATE_WAITING_FOR_ROUND_NUM_CHECK)
		printf("Both players are successfully connected!\n");
	
	//round num thing
	
	int p_round_num = round_num;
	
	//correction local check
	
	if (!is_round_num_correct(p_round_num))
		p_round_num = round_num_input_while(p_round_num);
	
	if (write(tcp_socket_fd, &p_round_num, sizeof(int)) <= 0)
		throw_exception_and_exit(tcp_socket_fd, 4);
	
	//correction server check
	
	server_answer serv_answ;

	if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
		throw_exception_and_exit(tcp_socket_fd, 4);
	
	if (serv_answ != SERVER_OK)
	{
		printf("Your input not received!\n");
		
		p_round_num = round_num_input_while(p_round_num);
		
		while(serv_answ != SERVER_OK)
		{
			if (write(tcp_socket_fd, &p_round_num, sizeof(int)) <= 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
			
			if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
			
			if (serv_answ != SERVER_OK)
			{
				printf("Your input not received!\n");
				p_round_num = round_num_input_while(p_round_num);
			}
		}
	}
	
	printf("Your input received!\n");
	
	//round nums dont match check
	
	if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exception_and_exit(tcp_socket_fd, 4);

	if (state == CLIENT_STATE_CORRECT_ROUND_NUM)
		printf("Server confirmed that both players' round numbers are correct!\n");
	else if (state == CLIENT_STATE_ROUND_NUMS_DONT_MATCH)
	{
		//priority generation
		
		if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
			throw_exception_and_exit(tcp_socket_fd, 4);
		
		//TRYING TO AGREE ON THE NUMBER OF ROUNDS
		
		if (state == CLIENT_STATE_HAVE_ROUND_NUM_PRIORITY) //IF PLAYER HAVE PRIORITY
		{
			int p2_round_num, agreement_input;
			
			if (read(tcp_socket_fd, &p2_round_num, sizeof(int)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
			
			printf("Do you agree with your opponent's round number? This number is: %i\n", p2_round_num);
			
			printf("If you do, please enter 1, if you dont - 0\n");
			scanf("%i", &agreement_input);
			printf("\n");
			
			if (!is_agreement_input_correct(agreement_input))
				agreement_input = agreement_input_while(agreement_input);
			
			if (write(tcp_socket_fd, &agreement_input, sizeof(int)) <= 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
		
			if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
			
			if (serv_answ == SERVER_OK)
				printf("Server recieved input\n");
			else
			{
				printf("Your input not received!\n");
				
				agreement_input = agreement_input_while(agreement_input);
				
				while (serv_answ != SERVER_OK)
				{
					if (write(tcp_socket_fd, &agreement_input, sizeof(int)) <= 0)
						throw_exception_and_exit(tcp_socket_fd, 4);
			
					if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
						throw_exception_and_exit(tcp_socket_fd, 4);
				
					if (serv_answ != SERVER_OK)
					{
						printf("Your input not received!\n");
						agreement_input = agreement_input_while(agreement_input);
					}	
				}
			
				printf("Your input received!\n");
			}
			
			//REACTION FOR DISAGREE
			
			if (agreement_input == 1)
			{
				printf("The game will start with p2 round num!\n");
				p_round_num = p2_round_num;
			}
			else
			{
				printf("Enter the new round num!\n");
				
				p_round_num = round_num_input_while(p_round_num);
				
				if (write(tcp_socket_fd, &p_round_num, sizeof(int)) <= 0)
					throw_exception_and_exit(tcp_socket_fd, 4);			
			
				if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
					throw_exception_and_exit(tcp_socket_fd, 4);
				
				if (serv_answ != SERVER_OK)
				{
					p_round_num = round_num_input_while(p_round_num);
					
					while(serv_answ != SERVER_OK)
					{
						if (write(tcp_socket_fd, &p_round_num, sizeof(int)) <= 0)
							throw_exception_and_exit(tcp_socket_fd, 4);
			
						if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
							throw_exception_and_exit(tcp_socket_fd, 4);
			
						if (serv_answ != SERVER_OK)
						{
							printf("Your input not received!\n");
							p_round_num = round_num_input_while(p_round_num);
						}	
					}
				}
				
				printf("Your input received!\n");
				
				if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
					throw_exception_and_exit(tcp_socket_fd, 4);
			
				if (state == CLIENT_STATE_WAITING_FOR_ANOTHER_PLAYER)
					printf("Waiting for answer from another player...\n");
				
				// REQUEST THE CONSENT OF THE SECOND PLAYER WITH A NEW NUMBER OF ROUNDS

				//REACTION FOR DISAGREE
				
				if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
					throw_exception_and_exit(tcp_socket_fd, 4);
					
				if (state == CLIENT_STATE_ERROR_WITH_AGREEMENT_OF_ROUND_NUMS)
				{
					printf("Something goes wrong with round nums. Please, restart the game!\n");
					throw_exception_and_exit(tcp_socket_fd, 4);					
				}
				else if (state == CLIENT_STATE_CORRECT_ROUND_NUM)
					printf("Server confirmed that both players' round numbers are correct!\n");
			}
		}
		else //IF PLAYER HAVE NO PRIORITY
		{
			if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
					throw_exception_and_exit(tcp_socket_fd, 4);
				
			// REQUEST THE CONSENT OF THE FIRST PLAYER WITH A NEW NUMBER OF ROUNDS
			
			if (state == CLIENT_STATE_HAVE_ROUND_NUM_PRIORITY)
			{
				int p1_round_num, agreement_input_final;
				
				if (read(tcp_socket_fd, &p1_round_num, sizeof(int)) < 0)
					throw_exception_and_exit(tcp_socket_fd, 4);
			
				printf("Do you agree with your opponent's round number? This number is: %i\n", p1_round_num);
			
				printf("If you do, please enter 1, if you dont - 0\n");
				scanf("%i", &agreement_input_final);
				printf("\n");
			
				if (!is_agreement_input_correct(agreement_input_final))
					agreement_input_final = agreement_input_while(agreement_input_final);
				
				if (write(tcp_socket_fd, &agreement_input_final, sizeof(int)) <= 0)
					throw_exception_and_exit(tcp_socket_fd, 4);
		
				if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) < 0)
					throw_exception_and_exit(tcp_socket_fd, 4);
			
				if (serv_answ == SERVER_OK)
					printf("Server recieved input\n");
				else
				{
					agreement_input_final = agreement_input_while(agreement_input_final);
				
					while (serv_answ != SERVER_OK)
					{
						if (write(tcp_socket_fd, &agreement_input_final, sizeof(int)) <= 0)
							throw_exception_and_exit(tcp_socket_fd, 4);
			
						if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
							throw_exception_and_exit(tcp_socket_fd, 4);
				
						if (serv_answ != SERVER_OK)
						{
							printf("Your input not received!\n");
							agreement_input_final = agreement_input_while(agreement_input_final);
						}	
					}
			
					printf("Your input received!\n");
				}
				
				//REACTION FOR DISAGREE
				
				if (agreement_input_final == 0)
				{
					if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
						throw_exception_and_exit(tcp_socket_fd, 4);
					
					if (state == CLIENT_STATE_ERROR_WITH_AGREEMENT_OF_ROUND_NUMS)
					{
						printf("Something goes wrong with round nums. Please, restart the game!\n");
						throw_exception_and_exit(tcp_socket_fd, 4);					
					}
				}
				else
				{
					if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
						throw_exception_and_exit(tcp_socket_fd, 4);
					
					if (state == CLIENT_STATE_CORRECT_ROUND_NUM)
						printf("Server confirmed that both players' round numbers are correct!\n");
					
					p_round_num = p1_round_num;
				}			
			}
		}
	}
	
	//initialization section
	
	if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
		throw_exception_and_exit(tcp_socket_fd, 4);
	
	if (state == CLIENT_STATE_WAITING_FOR_MASTER)
		printf("Waiting for the end of initialization and for master-thread...\n");
	
	//game threads section
	
	//selecting the starting player in the master thread
	
	char player_id;	
	
	if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exception_and_exit(tcp_socket_fd, 4);	
	
	if (state == CLIENT_STATE_WAIT_TURN)
	{
		printf("You are second player!\n");
		player_id == 1;
		printf("Waiting for another player...\n");
	}
	else if (state == CLIENT_STATE_MAKE_TURN)
	{
		printf("You are starting player!\n");
		player_id == 0;
	}
	
	//cycle of rounds
	
	finger finger_num = 0;
	int p_score, score;
	int step = 0;
	
	while (step < round_num)
	{
		if (state == CLIENT_STATE_MAKE_TURN)
		{
			finger_num = finger_input_while(finger_num);
	
			if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
	
			if (serv_answ != SERVER_OK)
			{
				while(serv_answ != SERVER_OK)
				{
					if (write(tcp_socket_fd, &finger_num, sizeof(finger)) < 0)
						throw_exception_and_exit(tcp_socket_fd, 4);
			
					if (read(tcp_socket_fd, &serv_answ, sizeof(server_answer)) < 0)
						throw_exception_and_exit(tcp_socket_fd, 4);
			
					if (serv_answ != SERVER_OK)
					{
						printf("Your input not received!\n");
						finger_num = finger_input_while(finger_num);
					}			
				}
			
				printf("Your input received!\n");
			}
	
			step++;
	
			if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);	
		}
		else if (state == CLIENT_STATE_WAITING_FOR_MASTER)
		{
			if (read(tcp_socket_fd, &score, sizeof(int)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
		
			printf("The current score is: %i\n", score);
		
			int p_score_temp;
		
			if (read(tcp_socket_fd, &p_score_temp, sizeof(int)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
		
			if (p_score_temp > p_score)
				printf("You won the round %i!\n", step + 1);
			else
				printf("You lost the round %i!\n", step + 1);
		
			p_score = p_score_temp;
			printf("Your curent score is: %i\n", p_score);
		
			if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
		
			if (state == CLIENT_STATE_GAME_WIN)
			{
				printf("=========== RESULTS ===========\n");
				printf("You won the game!\n");
				printf("===============================\n");
				break;
			}
			else if (state == CLIENT_STATE_GAME_LOSS)
			{
				printf("=========== RESULTS ===========\n");
				printf("You lost the game!\n");
				printf("===============================\n");
				break;
			}
		}
		else
		{
			printf("Waiting for anoher player's turn...\n");
		
			if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
				throw_exception_and_exit(tcp_socket_fd, 4);
		}
	}
	
	//socket close section
	
	close(tcp_socket_fd);
	
	exit(0);
}

int main(int argc, char* argv[])
{
	srand(time(NULL));
	
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