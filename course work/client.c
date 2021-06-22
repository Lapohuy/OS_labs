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
#include "pthread.h"
#include <pthread.h>

#include "utility.h"

typedef struct master_info_struct_client
{
    int p_score_temp;
    int p_score;
    int c_score_temp;
    int c_score;

    int round_num;

    int sem_id;
} master_info_client;

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

    if (state == CLIENT_STATE_MAKE_TURN)
		printf("You turn!\n");

    //cycle of rounds

    finger finger_num;
    int step = 0;

  while (step < round_num)
	{
    if (state == CLIENT_STATE_MAKE_TURN)
		{
		  printf("Please, write the number of fingers between 1 and 3 including!\n");
			scanf("%i", (int*) &finger_num);
			
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

      if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
		    throw_exception_and_exit(tcp_socket_fd, CLIENT_MAIN);

      if (state == CLIENT_STATE_WAITING_FOR_MASTER)
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
      else
        printf("Its your turn!\n");

      step++;
    }
  }

    //socket close section

    close(tcp_socket_fd);
	
	exit(0);
}

int get_rand_int_frome_range(int min, int max)
{
  if (!min)
    return rand() % (max + 1);
  
  return min + rand() + (max - min + 1);
}

void* p_thread(void* master_struct)
{
  master_info_client* info = (master_info_client*) master_struct;

  int step = 0;
  while (step < info->round_num)
  {
    sem_set_state(info->sem_id, PLAYER_ONE, LOCK);

    finger finger_num;

    printf("Please, write the number of fingers between 1 and 3 including!\n");
		scanf("%i", (int*) &finger_num);

    //need the upgrade

    if (finger_num >= 1 && finger_num <= 3)
      printf("Success!\n");
    else
    {
      printf("Wrong input!\n");
      print_programm_use_and_exit();
    }

    //need the upgrade

    info->p_score_temp = finger_num;

    step++;

    sem_set_state(info->sem_id, MASTER_ONE, UNLOCK);
  }

  return 0;
}

void* c_thread(void* master_struct)
{
  master_info_client* info = (master_info_client*) master_struct;

  int step = 0;
  while (step < info->round_num)
  {
    sem_set_state(info->sem_id, PLAYER_TWO, LOCK);

    finger finger_num = get_rand_int_frome_range(1, 3);

    info->c_score_temp = finger_num;

    step++;

    sem_set_state(info->sem_id, MASTER_TWO, UNLOCK);
  }

  return 0;
}

void* m_thread(void* master_struct)
{
  master_info_client* info = (master_info_client*) master_struct;

  int step = 0;
  while (step < info->round_num)
  {
    sem_set_state(info->sem_id, MASTER_ONE, LOCK);
    sem_set_state(info->sem_id, MASTER_TWO, LOCK);

    int score_temp = info->p_score_temp + info->c_score_temp;

    if (score_temp % 2 == 0)
    {
      printf("Player wins the round %i!\n", step + 1);
      info->p_score -= score_temp;
      info->c_score += score_temp;
    }
    else
    {
      printf("Computer wins the round %i!\n", step + 1);
      info->p_score += score_temp;
      info->c_score -= score_temp;
    }

    info->p_score_temp = info->c_score_temp = 0;

    if (step == info->round_num - 1)
    {
      if (info->c_score > info->p_score)
      {
        printf("=========== RESULTS ===========\n");
        printf("Computer wins the game!\n");
        printf("===============================\n");
                
        break;
      }
      else
      {
        printf("=========== RESULTS ===========\n");
        printf("Player wins the game!\n");
        printf("===============================\n");

        break;
      }
    }

    step++;

    sem_set_state(info->sem_id, PLAYER_ONE, UNLOCK);
    sem_set_state(info->sem_id, PLAYER_TWO, UNLOCK);
  }

  return 0;
}

int main(int argc, char* argv[])
{
  srand(time(NULL));

  int round_num;
    
  if (argc == 2)
	{
		//need the upgrade

    round_num = atoi(argv[1]);
    if (round_num >= 1 && round_num <= 1000)
      printf("Success!\n");
    else
      print_programm_use_and_exit();

    //need the upgrade
	}
	else if (argc == 4)
	{
		int round_num_index_from_argv = 1;
		char ip_index_from_argv = 2;
		char port_index_from_argv = 3;
		
		player_versus_player(argv[ip_index_from_argv], atoi(argv[port_index_from_argv]), atoi(argv[round_num_index_from_argv]));	
	}
	else
		print_programm_use_and_exit();

  master_info_client info;

  info.round_num = round_num;

  info.sem_id = semget(IPC_PRIVATE, 4, 0600 | IPC_CREAT);
  if (info.sem_id < 0)
  {
      perror("Error with semget()!\n");
      print_programm_use_and_exit();
  }
  sem_set_state(info.sem_id, PLAYER_ONE, UNLOCK);
  sem_set_state(info.sem_id, PLAYER_TWO, UNLOCK);

  printf("Starting the game...\n");

  int master_thread_returned_value;
  pthread_t player_one_thread;
  pthread_t master_thread;
  pthread_t player_two_thread;

  pthread_create(&player_one_thread, NULL, p_thread, &info);
  pthread_create(&master_thread, NULL, m_thread, &info); 
  pthread_create(&player_two_thread, NULL, c_thread, &info);	  

  pthread_join(master_thread, (void**) &master_thread_returned_value);

  printf("Master thread exit with status: %i\n", master_thread_returned_value);

  printf("Shutting down the client...\n");

  free_semaphores(&info.sem_id);  
	
	return 0;
}