#include "utility.h"

char is_player_input_correct(finger finger_num)
{
    if ((finger_num == 1) || (finger_num == 2) || (finger_num == 3))
        return 1;
    else
        return 0;
}

char is_round_num_correct(int round_num)
{
	if ((round_num > 0) && (round_num <= 1000))
		return 1;
	else
		return 0;
}

char get_random_char_from_range(char min, char max)
{
    if (!min)
        return rand() % (max + 1);
    return min + rand() + (max - min + 1);
}

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
    exit(1);
}

char is_agreement_input_correct(int input)
{
	if ((input == 0) || (input == 1))
		return 1;
	else
		return 0;
}