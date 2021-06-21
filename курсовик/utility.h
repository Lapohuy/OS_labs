#ifndef UTILITY_H
#define UTILITY_H

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/sem.h"

#define UNLOCK 1
#define LOCK -1

typedef struct sockaddr socket_address;
typedef struct sockaddr_in socket_address_in;

typedef enum contributors_id_enum
{
    PLAYER_ONE_ID,
    PLAYER_TWO_ID,
    MASTER_ID,
} contributors_id;

typedef short finger;

typedef enum server_answer_enum
{
    SERVER_OK,
    SERVER_ERROR
} server_answer;

typedef enum client_states_enum
{
    CLIENT_STATE_WAITING_FOR_ANOTHER_PLAYER,
	CLIENT_STATE_CORRECT_ROUND_NUM,
	CLIENT_STATE_ROUND_NUMS_DONT_MATCH,
	CLIENT_STATE_HAVE_ROUND_NUM_PRIORITY,
    CLIENT_STATE_WAITING_FOR_ROUND_NUM_CHECK,
	CLIENT_STATE_ERROR_WITH_AGREEMENT_OF_ROUND_NUMS,
    CLIENT_STATE_WAITING_FOR_MASTER,
    CLIENT_STATE_WAIT_TURN,
    CLIENT_STATE_MAKE_TURN,
    CLIENT_STATE_GAME_WIN,
    CLIENT_STATE_GAME_LOSS

} client_states;

char is_player_input_correct(finger finger_num);

char is_round_num_correct(int round_num);

char get_random_char_from_range(char min, char max);

void sem_set_state(int sem_id, int num, int state);

void throw_exception_and_exit(u_int16_t tcp_socker_fd, char thread_num);

char is_agreement_input_correct(int input);

#endif