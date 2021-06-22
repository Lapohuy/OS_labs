#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "pthread.h"
#include <pthread.h>
#include "sys/types.h"
#include "sys/socket.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include <strings.h>

#include "utility.h"

typedef struct master_info_struct
{
    u_int16_t p1_tcp_socket_fd;
    u_int16_t p2_tcp_socket_fd;
    
    client_states p1_own_state;
    client_states p2_own_state;

    int p1_score_temp;
    int p1_score;
    int p2_score_temp;
    int p2_score;

    int round_num;

    int sem_id;
} master_info;

void print_server_port(const u_int16_t tcp_socket_fd)
{
    struct sockaddr_in socket_address;
    socklen_t socket_len = sizeof(socket_address);

    getsockname(tcp_socket_fd, (struct sockaddr*) &socket_address, &socket_len);
    printf("Server started in %i port\n", ntohs(socket_address.sin_port));
}

void print_help_and_exit()
{
    printf("========================= REQUIRED INPUT =========================\n");
    printf("| <nothing> == To start game with any port                       |\n");
    printf("| <port> == To start with selected port                          |\n");
	printf("| The correct number of rounds is from 1 to 1000                 |\n");
    printf("==================================================================\n");
    exit(0);
}

void* p1_thread(void* master_struct)
{
    master_info* info = (master_info*) master_struct;
    server_answer serv_answ;

    int step = 0;
    while (step < info->round_num)
    {
        sem_set_state(info->sem_id, PLAYER_ONE, LOCK);

        finger finger_num;

        serv_answ = SERVER_ERROR;

        while(serv_answ != SERVER_OK)
        {
            if (read(info->p1_tcp_socket_fd, &finger_num, sizeof(finger)) <= 0)
                throw_exception_and_exit(info->p1_tcp_socket_fd, PLAYER_ONE);
            
            if (finger_num <= 3 && finger_num >= 1)
            {
                serv_answ = SERVER_OK;

                if (write(info->p1_tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
                    throw_exception_and_exit(info->p1_tcp_socket_fd, PLAYER_ONE);
            }
            else //else if (finger_num > 3 || finger_num < 1)
            {
                if (write(info->p1_tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
                    throw_exception_and_exit(info->p1_tcp_socket_fd, PLAYER_ONE);
            }
        }

        info->p1_score_temp = finger_num;

        info->p1_own_state = CLIENT_STATE_WAITING_FOR_MASTER;

        if (write(info->p1_tcp_socket_fd, &info->p1_own_state, sizeof(client_states)) <= 0)
            throw_exception_and_exit(info->p1_tcp_socket_fd, PLAYER_ONE);
        
        step++;      

        sem_set_state(info->sem_id, MASTER_ONE, UNLOCK);
    }

    return 0;
}

void* p2_thread(void* master_struct)
{
    master_info* info = (master_info*) master_struct;
    server_answer serv_answ;

    int step = 0;
    while (step < info->round_num)
    {
        sem_set_state(info->sem_id, PLAYER_TWO, LOCK);

        finger finger_num;

        serv_answ = SERVER_ERROR;

        while(serv_answ != SERVER_OK)
        {
            if (read(info->p2_tcp_socket_fd, &finger_num, sizeof(finger)) <= 0)
                throw_exception_and_exit(info->p2_tcp_socket_fd, PLAYER_TWO);
            
            if (finger_num <= 3 && finger_num >= 1)
            {
                serv_answ = SERVER_OK;

                if (write(info->p2_tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
                    throw_exception_and_exit(info->p2_tcp_socket_fd, PLAYER_TWO);
            }
            else //else if (finger_num > 3 || finger_num < 1)
            {
                if (write(info->p2_tcp_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
                    throw_exception_and_exit(info->p2_tcp_socket_fd, PLAYER_TWO);
            }
        }

        info->p2_score_temp = finger_num;

        info->p2_own_state = CLIENT_STATE_WAITING_FOR_MASTER;

        if (write(info->p2_tcp_socket_fd, &info->p2_own_state, sizeof(client_states)) <= 0)
            throw_exception_and_exit(info->p2_tcp_socket_fd, PLAYER_TWO);

        step++;      

        sem_set_state(info->sem_id, MASTER_TWO, UNLOCK);
    }

    return 0;
}

void* m_thread(void* master_struct)
{
    master_info* info = (master_info*) master_struct;
    server_answer serv_answ;

    int step = 0;
    while (step < info->round_num)
    {
        sem_set_state(info->sem_id, MASTER_ONE, LOCK);
        sem_set_state(info->sem_id, MASTER_TWO, LOCK);

        int score_temp = info->p1_score_temp + info->p2_score_temp;

        if (score_temp % 2 == 0)
        {
            printf("Player two wins the round %i!\n", step + 1);
            info->p1_score -= score_temp;
            info->p2_score += score_temp;

            info->p2_own_state = CLIENT_STATE_ROUND_WIN;
            info->p1_own_state = CLIENT_STATE_ROUND_LOSS;
        }
        else
        {
            printf("Player one wins the round %i!\n", step + 1);
            info->p1_score += score_temp;
            info->p2_score -= score_temp;

            info->p1_own_state = CLIENT_STATE_ROUND_WIN;
            info->p2_own_state = CLIENT_STATE_ROUND_LOSS;
        }

        if (write(info->p1_tcp_socket_fd, &info->p1_own_state, sizeof(client_states)) <= 0)
            throw_exception_and_exit(info->p1_tcp_socket_fd, MASTER_ONE);
        
        if (write(info->p2_tcp_socket_fd, &info->p2_own_state, sizeof(client_states)) <= 0)
            throw_exception_and_exit(info->p2_tcp_socket_fd, MASTER_TWO);

        info->p1_score_temp = info->p2_score_temp = 0;

        if (step == info->round_num - 1)
        {
            if (info->p2_score > info->p1_score)
            {
                printf("=========== RESULTS ===========\n");
                printf("Player one wins the game!\n");
                printf("===============================\n");
                
                info->p1_own_state = CLIENT_STATE_GAME_WIN;
                info->p2_own_state = CLIENT_STATE_GAME_LOSS;
            }
            else
            {
                printf("=========== RESULTS ===========\n");
                printf("Player two wins the game!\n");
                printf("===============================\n");

                info->p2_own_state = CLIENT_STATE_GAME_WIN;
                info->p1_own_state = CLIENT_STATE_GAME_LOSS;
            }
        }
        else
            info->p1_own_state = info->p2_own_state = CLIENT_STATE_MAKE_TURN;

        if (write(info->p1_tcp_socket_fd, &info->p1_own_state, sizeof(client_states)) <= 0)
            throw_exception_and_exit(info->p1_tcp_socket_fd, MASTER_ONE);
        
        if (write(info->p2_tcp_socket_fd, &info->p2_own_state, sizeof(client_states)) <= 0)
            throw_exception_and_exit(info->p2_tcp_socket_fd, MASTER_TWO);
        
        if (step == info->round_num - 1)
            break;
        
        step++;

        sem_set_state(info->sem_id, PLAYER_ONE, UNLOCK);
        sem_set_state(info->sem_id, PLAYER_TWO, UNLOCK);       
    }

    return 0;    
}

int main(int argc, char* argv[])
{
	//sockets setup
	
	u_int16_t server_port;

    if (argc == 1)
        server_port = 0;
    else if (argc == 2)
        server_port = atoi(argv[1]);
    else
        print_help_and_exit();
    
    uint32_t received_bytes = 0;
	
	u_int16_t tcp_socket_fd;
    u_int16_t tcp_player_one_socket_fd;
    u_int16_t tcp_player_two_socket_fd;

    socket_address_in server_address;
    socket_address_in client_address;
	
	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));

    client_states f_client_state;
    client_states s_client_state;

    if ((tcp_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror(NULL);
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(tcp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
    {
        printf("Error! Can't get %i port! Tryig to get another!\n", server_port);

        server_address.sin_port = htons(0);
        if (bind(tcp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
        {
            perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }
    }

    print_server_port(tcp_socket_fd);

    if (listen(tcp_socket_fd, 1) < 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }

    tcp_player_one_socket_fd = accept(tcp_socket_fd, (socket_address*) &client_address, &received_bytes);

    if (tcp_player_one_socket_fd < 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }
	
	f_client_state = CLIENT_STATE_WAITING_FOR_ANOTHER_PLAYER;

    if (write(tcp_player_one_socket_fd, &f_client_state, sizeof(client_states)) < 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }

    printf("First player is successfully connected!\n");

    tcp_player_two_socket_fd = accept(tcp_socket_fd, (socket_address*) &client_address, &received_bytes);

    if (tcp_player_two_socket_fd < 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }

    if (write(tcp_player_two_socket_fd, &s_client_state, sizeof(client_states)) < 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }
	
	f_client_state = CLIENT_STATE_WAITING_FOR_GAME_START;
    s_client_state = CLIENT_STATE_WAITING_FOR_GAME_START;
	
	if (write(tcp_player_one_socket_fd, &f_client_state, sizeof(client_states)) < 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }
    
    if (write(tcp_player_two_socket_fd, &s_client_state, sizeof(client_states)) < 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }

    printf("Both players are successfully connected!\n");

    //round num check

    //need to upgrade
    
    int p1_round_num, p2_round_num, round_num;
	
	if (read(tcp_player_one_socket_fd, &p1_round_num, sizeof(int)) <= 0)
	{
		perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }
	
	if (read(tcp_player_two_socket_fd, &p2_round_num, sizeof(int)) <= 0)
	{
		perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }

    server_answer serv_answ;

    if (p1_round_num < 1 || p1_round_num > 1000)
    {
        serv_answ = SERVER_ERROR;
        
        if (write(tcp_player_one_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }

        print_help_and_exit();
    }
    else
    {
        serv_answ = SERVER_OK;
        
        if (write(tcp_player_one_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }
    }
    
    if (p2_round_num < 1 || p2_round_num > 1000)
    {
        serv_answ = SERVER_ERROR;
        
        if (write(tcp_player_two_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }

        print_help_and_exit();
    }
    else
    {
        serv_answ = SERVER_OK;
        
        if (write(tcp_player_two_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }
    }
        
    if (p1_round_num == p2_round_num)
    {
        round_num = p1_round_num;

        serv_answ = SERVER_OK;
        
        if (write(tcp_player_two_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }
        
        if (write(tcp_player_one_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }
    }
    else
    {
        serv_answ = SERVER_ERROR;
        
        if (write(tcp_player_two_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }
        
        if (write(tcp_player_one_socket_fd, &serv_answ, sizeof(server_answer)) <= 0)
	    {
		    perror(NULL);
            close(tcp_socket_fd);
            return -1;
        }

        print_help_and_exit();
    }

    //need to upgrade

    //initialization section

    master_info info;

    info.round_num = round_num;
    info.p1_tcp_socket_fd = tcp_player_one_socket_fd;
    info.p2_tcp_socket_fd = tcp_player_two_socket_fd;
    info.p1_own_state = f_client_state;
    info.p2_own_state = s_client_state;

    info.sem_id = semget(IPC_PRIVATE, 4, 0600 | IPC_CREAT);
    if (info.sem_id < 0)
    {
        perror("Error with semget()!\n");
        print_help_and_exit();
    }
    sem_set_state(info.sem_id, PLAYER_ONE, UNLOCK);
    sem_set_state(info.sem_id, PLAYER_TWO, UNLOCK);

    info.p1_own_state = info.p2_own_state = CLIENT_STATE_MAKE_TURN;

    if (write(info.p1_tcp_socket_fd, &info.p1_own_state, sizeof(client_states)) <= 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }

    if (write(info.p2_tcp_socket_fd, &info.p2_own_state, sizeof(client_states)) <= 0)
    {
        perror(NULL);
        close(tcp_socket_fd);
        return -1;
    }

    int master_thread_returned_value;

    pthread_t player_one_thread;
    pthread_t master_thread;
    pthread_t player_two_thread;    
    
    pthread_create(&player_one_thread, NULL, p1_thread, &info);
    pthread_create(&master_thread, NULL, m_thread, &info); 
    pthread_create(&player_two_thread, NULL, p2_thread, &info);	  
	
    pthread_join(master_thread, (void**) &master_thread_returned_value);
	
    printf("Master thread exit with status: %i\n", master_thread_returned_value);

    printf("Shutting down the server...\n");

    free_semaphores(&info.sem_id);
   
    return 0;
}