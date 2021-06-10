#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "pthread.h"
#include "signal.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "arpa/inet.h"
#include "netinet/in.h"

typedef struct sockaddr socket_address;
typedef struct sockaddr_in socket_address_in;

typedef enum contributors_id_enum
{
    PLAYER_ONE_ID,
    PLAYER_TWO_ID,
    MASTER_ID,
} contributors_id;

#define UNLOCK 1
#define LOCK -1

typedef short finger;

typedef enum server_answer_enum
{
    SERVER_OK,
    SERVER_ERROR
} server_answer;

typedef enum client_states_enum
{
    CLIENT_STATE_WAITING_FOR_ANOTHER_PLAYER,
    CLIENT_STATE_WAITING_FOR_MASTER,
    CLIENT_STATE_WAIT_TURN,
    CLIENT_STATE_MAKE_TURN,
    CLIENT_STATE_GAME_WIN,
    CLIENT_STATE_GAME_LOSS
} client_states;

typedef struct thread_player_info_struct
{
    client_states own_state;
    int p_score_temp;
    int p_score;
    u_int16_t tcp_socket_fd;
} thread_player_info;

typedef struct thread_master_info_struct
{
    int round_num;
    int sem_id;
    int score;
    contributors_id starting_player, active_contributor;
    thread_player_info thread_player_one_info;
    thread_player_info thread_player_two_info;
} thread_master_info;

void sem_set_state(int sem_id, int num, int state)
{
	struct sembuf op;
	op.sem_op = state;
	op.sem_flg = 0;
	op.sem_num = num;
	semop(sem_id, &op, 1);
}

void throw_exception_and_exit(u_int16_t tcp_socker_fd, char thread_num)
{
    printf("Can't read or send data. Thread number: %i. Shutting down the server...\n", thread_num);
    perror(NULL);
    close(tcp_socker_fd);
    edwin();
    exit(1);
}

char get_random_char_from_range(char min, char max)
{
    srand(time(NULL));
    if (!min)
        return rand() % (max + 1);
    return min + rand() + (max - min + 1);
}

void print_server_port(const u_int16_t tcp_socket_fd)
{
    struct sockaddr_in socket_address;
    socklen_t socket_len = sizeof(socket_address);

    getsockname(tcp_socket_fd, (struct sockaddr*) &socket_address, &socket_len);
    printf("Server started in %i port\n", ntohs(socket_address.sin_port));
}

char is_player_input_correct(finger finger_num)
{
    if ((finger_num == 1) || (finger_num == 2) || (finger_num == 3))
        return 1;
    else
        return 0;
}

void free_semaphores(int* sem_id)
{
    char resource_delete_command_buff[124];
    sprintf(resource_delete_command_buff, "ipcrm -s %i", *sem_id);
    system(resource_delete_command_buff);
    *sem_id = 0;
}

void print_help_and_exit()
{
    printf("========================= REQUIRED INPUT =========================\n");
    printf("| <R> == To start game with any port and R rounds                |\n");
    printf("| <port> <round_num> == To start with selected port and R rounds |\n");
    printf("| The correct range of the R is from 1 to 1000                   |\n");
    printf("==================================================================\n");
    exit(0);
}

void signal_handler(int nsig)
{
	if (nsig == SIGINT)
	{
		semctl(sem_id, 0, IPC_RMID, 0);
		exit(0);
	}
}

void* master_turns_thread(void* master_struct)
{
    signal(SIGINT, signal_handler);
	
	thread_master_info* master_info = (thread_player_info*) master_struct;
    server_answer serv_answ;

    sem_set_state(master_info->sem_id, MASTER_ID, LOCK);

    master_info->starting_player = get_random_char_from_range(0, 1);

    if (master_info->starting_player == PLAYER_ONE_ID)
    {
        master_info->thread_player_one_info.own_state = CLIENT_STATE_MAKE_TURN;
        master_info->thread_player_two_info.own_state = CLIENT_STATE_WAIT_TURN;
    }
    else {
        master_info->thread_player_two_info.own_state = CLIENT_STATE_MAKE_TURN;
        master_info->thread_player_one_info.own_state = CLIENT_STATE_WAIT_TURN;
    }

    if (write(master_info->thread_player_one_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
        throw_exception_and_exit(master_info->thread_player_one_info.tcp_socket_fd, MASTER_ID);

    if (write(master_info->thread_player_two_info.tcp_socket_fd, master_info->thread_player_two_info.own_state, sizeof(client_states)) <= 0)
        throw_exception_and_exit(master_info->thread_player_two_info.tcp_socket_fd, MASTER_ID);

    master_info->active_contributor = master_info->starting_player;
    sem_set_state(master_info->sem_id, master_info->active_contributor, UNLOCK);

    int step = 0;

    while (step < master_info->round_num)
    {
        sem_set_state(master_info->sem_id, MASTER_ID, LOCK);
        int s1_temp = master_info->thread_player_one_info.p_score_temp;
        int s2_temp = master_info->thread_player_two_info.p_score_temp;
        
        int s3_temp = s1_temp + s2_temp;
        master_info->score = s3_temp;        

        if (s3_temp % 2 == 0)
        {
            printf("Player two wins the round %i!\n", step + 1);
            if (master_info->starting_player == PLAYER_ONE_ID)
            {
                master_info->thread_player_two_info.p_score += s3_temp;
                master_info->thread_player_one_info.p_score -= s3_temp;
            }
            else if (master_info->starting_player == PLAYER_TWO_ID)
            {
                master_info->thread_player_one_info.p_score += s3_temp;
                master_info->thread_player_two_info.p_score -= s3_temp;
            }
        }
        else
        {
            printf("Player one wins the round %i!\n", step + 1);
            if (master_info->starting_player == PLAYER_ONE_ID)
            {
                master_info->thread_player_one_info.p_score += s3_temp;
                master_info->thread_player_two_info.p_score -= s3_temp;
            }
            else if (master_info->starting_player == PLAYER_TWO_ID)
            {
                master_info->thread_player_two_info.p_score += s3_temp;
                master_info->thread_player_one_info.p_score -= s3_temp;
            }
        }

        master_info->thread_player_one_info.p_score_temp = master_info->thread_player_two_info.p_score_temp = 0;

        step += 1;

        if (step == master_info->round_num - 1)
        {
            if (master_info->thread_player_one_info.p_score > master_info->thread_player_two_info.p_score)
            {
                printf("=========== RESULTS ===========\n");
                printf("Player one wins the game!\n");
                printf("===============================\n");
                master_info->thread_player_one_info.own_state = CLIENT_STATE_GAME_WIN;
                master_info->thread_player_two_info.own_state = CLIENT_STATE_GAME_LOSS;
            }
            else
            {
                printf("=========== RESULTS ===========\n");
                printf("Player two wins the game!\n");
                printf("===============================\n");
                master_info->thread_player_two_info.own_state = CLIENT_STATE_GAME_WIN;
                master_info->thread_player_one_info.own_state = CLIENT_STATE_GAME_LOSS;
            }
            
            if (write(master_info->thread_player_one_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                throw_exception_and_exit(master_info->thread_player_one_info.tcp_socket_fd, MASTER_ID);
            
            if (write(master_info->thread_player_two_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                throw_exception_and_exit(master_info->thread_player_two_info.tcp_socket_fd, MASTER_ID);
            
            break;
        }
        else
        {
            if (master_info->starting_player == PLAYER_ONE_ID)
            {
                master_info->thread_player_two_info.own_state = CLIENT_STATE_WAIT_TURN;
                master_info->thread_player_one_info.own_state = CLIENT_STATE_MAKE_TURN;
            }
            else if (master_info->starting_player == PLAYER_TWO_ID)
            {
                master_info->thread_player_one_info.own_state = CLIENT_STATE_WAIT_TURN;
                master_info->thread_player_two_info.own_state = CLIENT_STATE_MAKE_TURN;
            }
        }

        if (write(master_info->thread_player_one_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                throw_exception_and_exit(master_info->thread_player_one_info.tcp_socket_fd, MASTER_ID);
            
        if (write(master_info->thread_player_two_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                throw_exception_and_exit(master_info->thread_player_two_info.tcp_socket_fd, MASTER_ID);

        master_info->active_contributor = master_info->starting_player;
        sem_set_state(master_info->sem_id, master_info->active_contributor, UNLOCK);        
    }

    return 0;
}

void* user_turns_thread(void* master_struct)
{
    signal(SIGINT, signal_handler);
	
	thread_master_info* master_info = (thread_player_info*) master_struct;
    server_answer serv_answ = SERVER_ERROR;

    int step = 0;

    u_int16_t tcp_current_player_id;
    if (master_info->active_contributor == PLAYER_ONE_ID)
        tcp_current_player_id = master_info->thread_player_one_info.tcp_socket_fd;
    else if (master_info->active_contributor == PLAYER_TWO_ID)
        tcp_current_player_id = master_info->thread_player_two_info.tcp_socket_fd;

    while (step < master_info->round_num)
    {
        sem_set_state(master_info->sem_id, master_info->active_contributor, LOCK);

        finger finger_num;

        while(serv_answ != SERVER_OK)
        {
            if (read(tcp_current_player_id, &finger_num, sizeof(finger)) <= 0)
                throw_exception_and_exit(tcp_current_player_id, master_info->active_contributor);
            
            if (!is_player_input_correct(finger_num))
            {
                if (write(tcp_current_player_id, &serv_answ, sizeof(server_answer)) <= 0)
                    throw_exception_and_exit(tcp_current_player_id, master_info->active_contributor);
                continue;
            }

            serv_answ = SERVER_OK;

            if (write(tcp_current_player_id, &serv_answ, sizeof(server_answer)) <= 0)
                    throw_exception_and_exit(tcp_current_player_id, master_info->active_contributor);
        }

        step++;

        if (master_info->starting_player == PLAYER_ONE_ID)
        {
            if (master_info->active_contributor == PLAYER_ONE_ID)
            {
                master_info->thread_player_one_info.own_state = CLIENT_STATE_WAIT_TURN;
                master_info->thread_player_two_info.own_state = CLIENT_STATE_MAKE_TURN;
                
                if (write(master_info->thread_player_one_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_one_info.tcp_socket_fd, master_info->active_contributor);
            
                if (write(master_info->thread_player_two_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_two_info.tcp_socket_fd, master_info->active_contributor);
                master_info->active_contributor = PLAYER_TWO_ID;
            }
            else if (master_info->active_contributor == PLAYER_TWO_ID)
            {
                master_info->thread_player_one_info.own_state = CLIENT_STATE_WAITING_FOR_MASTER;
                master_info->thread_player_two_info.own_state = CLIENT_STATE_WAITING_FOR_MASTER;
                if (write(master_info->thread_player_one_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_one_info.tcp_socket_fd, master_info->active_contributor);
            
                if (write(master_info->thread_player_two_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_two_info.tcp_socket_fd, master_info->active_contributor);
                master_info->active_contributor = MASTER_ID;
            }
        }
        else if (master_info->starting_player == PLAYER_TWO_ID)
        {
            if (master_info->active_contributor == PLAYER_ONE_ID)
            {
                master_info->thread_player_one_info.own_state = CLIENT_STATE_WAITING_FOR_MASTER;
                master_info->thread_player_two_info.own_state = CLIENT_STATE_WAITING_FOR_MASTER;
                if (write(master_info->thread_player_one_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_one_info.tcp_socket_fd, master_info->active_contributor);
            
                if (write(master_info->thread_player_two_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_two_info.tcp_socket_fd, master_info->active_contributor);
                master_info->active_contributor = MASTER_ID;
            }
            else if (master_info->active_contributor == PLAYER_TWO_ID)
            {
                master_info->thread_player_one_info.own_state = CLIENT_STATE_MAKE_TURN;
                master_info->thread_player_two_info.own_state = CLIENT_STATE_WAIT_TURN;
                if (write(master_info->thread_player_one_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_one_info.tcp_socket_fd, master_info->active_contributor);
            
                if (write(master_info->thread_player_two_info.tcp_socket_fd, master_info->thread_player_one_info.own_state, sizeof(client_states)) <= 0)
                    throw_exception_and_exit(master_info->thread_player_two_info.tcp_socket_fd, master_info->active_contributor);
                master_info->active_contributor = PLAYER_ONE_ID;
            }
        }

        sem_set_state(master_info->sem_id, master_info->active_contributor, UNLOCK);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, signal_handler);
	
	u_int16_t server_port;

    if (argc == 2)
        server_port = 0;
    else if (argc == 3)
        server_port = atoi(argv[1]);
    else
        print_help_and_exit();
    
    uint32_t received_bytes = 0;

    u_int16_t tcp_socket_fd;
    u_int16_t tcp_player_one_socket_fd;
    u_int16_t tcp_player_two_socket_fd;

    socket_address_in server_address;
    socket_address_in client_address;

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

    if (listens(tcp_socket_fd, 1) < 0)
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

    printf("Both players are successfully connected!\n");

    f_client_state = CLIENT_STATE_WAITING_FOR_MASTER;
    s_client_state = CLIENT_STATE_WAITING_FOR_MASTER;

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

    thread_master_info master_info;

    if (argc == 2)
        master_info.round_num = atoi(argv[1]);
    else if (argc == 3)
        master_info.round_num = atoi(argv[2]);

    if ((master_info.round_num < 1) || (master_info.round_num > 1000))
        print_help_and_exit();

    master_info.sem_id = semget(IPC_PRIVATE, 2, 0600 | IPC_CREAT);
    if (master_info.sem_id < 0)
    {
        perror("Error with semget()!\n");
        print_help_and_exit();
    }
    sem_set_state(master_info.sem_id, MASTER_ID, UNLOCK);
    sem_set_state(master_info.sem_id, PLAYER_ONE_ID, LOCK);
    sem_set_state(master_info.sem_id, PLAYER_TWO_ID, LOCK);
    master_info.thread_player_one_info.tcp_socket_fd = tcp_player_one_socket_fd;
    master_info.thread_player_two_info.tcp_socket_fd = tcp_player_two_socket_fd;
    master_info.thread_player_one_info.own_state = &f_client_state;
    master_info.thread_player_two_info.own_state = &s_client_state;
    master_info.active_contributor = MASTER_ID;

    int master_thread_returned_value;
    int player_one_thread_returned_value;
    int player_two_thread_returned_value;

    pthread_t master_thread;
    pthread_t player_one_thread;
    pthread_t player_two_thread;

    pthread_create(&master_thread, NULL, master_turns_thread, &master_info);
    pthread_create(&player_one_thread, NULL, user_turns_thread, &master_info);
    pthread_create(&player_two_thread, NULL, user_turns_thread, &master_info);
    
    pthread_join(master_thread, (void**) &master_thread_returned_value);
    pthread_join(player_one_thread, (void**) &player_one_thread_returned_value);
    pthread_join(player_two_thread, (void**) &player_two_thread_returned_value);

    printf("First thread exit with status: %i\n", master_thread_returned_value);
    printf("Second thread exit with status: %i\n", player_one_thread_returned_value);
    printf("Third thread exit with status: %i\n", player_two_thread_returned_value);

    prinf("Shutting down the server...\n");

    free_semaphores(&master_info.sem_id);
   
    return 0;
}