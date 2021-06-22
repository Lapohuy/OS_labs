#include "utility.h"

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

void free_semaphores(int* sem_id)
{
    char resource_delete_command_buff[124];
    sprintf(resource_delete_command_buff, "ipcrm -s %i", *sem_id);
    system(resource_delete_command_buff);
    *sem_id = 0;
}