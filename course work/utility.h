#ifndef UTILITY_H
#define UTILITY_H

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/sem.h"

#define UNLOCK 1
#define LOCK -1

#define PLAYER_ONE 0
#define PLAYER_TWO 1
#define MASTER_ONE 2
#define MASTER_TWO 3
#define CLIENT_MAIN 4

typedef struct sockaddr socket_address;
typedef struct sockaddr_in socket_address_in;

typedef struct sockaddr socket_address;
typedef struct sockaddr_in socket_address_in;

typedef int finger;

typedef enum server_answer_enum
{
    SERVER_OK,
    SERVER_ERROR
} server_answer;

typedef enum client_states_enum
{
    CLIENT_STATE_WAITING_FOR_ANOTHER_PLAYER,
    CLIENT_STATE_WAITING_FOR_GAME_START,
    CLIENT_STATE_MAKE_TURN,
    CLIENT_STATE_WAITING_FOR_MASTER,
    CLIENT_STATE_ROUND_WIN,
    CLIENT_STATE_ROUND_LOSS,
    CLIENT_STATE_GAME_WIN,
    CLIENT_STATE_GAME_LOSS
} client_states;

void sem_set_state(int sem_id, int num, int state);

void throw_exception_and_exit(u_int16_t tcp_socker_fd, char thread_num);

void free_semaphores(int* sem_id);

#endif