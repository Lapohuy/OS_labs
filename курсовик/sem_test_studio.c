#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "pthread.h"

#include "sys/types.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include <sys/ipc.h>

#define UNLOCK 1
#define LOCK -1

struct game_info
{
	int R, sem_id;
	s_temp_arr[2];
	s_arr[3];
};

void sem_set_state(int sem_id, int num, int state)
{
	struct sembuf op;
	op.sem_op = state;
	op.sem_flg = 0; // !!!
	op.sem_num = num;
	semop(sem_id, &op, 1);
}

void* user1_turn(struct game_info* g_i)
{
	int step1 = 0;
	while (step1 < g_i[0].R)
	{
		int local;
		printf("Enter the number between 1-3\n");
		scanf("%d", &local);
		printf("local_user1=%d\n", local);
		g_i[0].s_temp_arr[0] = local;
		step1++;
		sem_set_state(g_i[0].sem_id, 0, LOCK);
		sem_set_state(g_i[0].sem_id, 1, UNLOCK);
	}
}

void* user2_turn(struct game_info* g_i)
{
	int step2 = 0;
	while (step2 < g_i[0].R)
	{
		int local;
		printf("Enter the number between 1-3\n");
		scanf("%d", &local);
		printf("local_user2=%d\n", local);
		g_i[0].s_temp_arr[1] = local;
		step2++;
		sem_set_state(g_i[0].sem_id, 1, LOCK);
		sem_set_state(g_i[0].sem_id, 2, UNLOCK);
	}
}

void* master_turn(struct game_info* g_i)
{
	int step3 = 0;
	while (step3 < g_i[0].R)
	{
		g_i[0].s_arr[0] += g_i[0].s_temp_arr[0] + g_i[0].s_temp_arr[1];
		g_i[0].s_temp_arr[0] = g_i[0].s_temp_arr[1] = 0;
		printf("s0 = %d\n", g_i[0].s_arr[0]);
		if (g_i[0].s_arr[0] % 2 == 0) {
			printf("User2 wins!\n");
			g_i[0].s_arr[1] -= g_i[0].s_arr[0];
			g_i[0].s_arr[2] += g_i[0].s_arr[0];
		}
		else
		{
			printf("User1 wins!\n");
			g_i[0].s_arr[1] += g_i[0].s_arr[0];
			g_i[0].s_arr[2] -= g_i[0].s_arr[0];
		}
        step3++;		
		sem_set_state(g_i[0].sem_id, 2, LOCK);
		sem_set_state(g_i[0].sem_id, 0, UNLOCK);
	}
}

int main()
{
	struct game_info g_i[1];
    
    g_i[0].sem_id = semget(IPC_PRIVATE, 3, 0600 | IPC_CREAT);
	g_i[0].R = 4;

	if (g_i[0].sem_id < 0)
	{
		perror("Sem err\n");
		return -1;
	}
	
	sem_set_state(g_i[0].sem_id, 0, UNLOCK);
	sem_set_state(g_i[0].sem_id, 1, LOCK);
	sem_set_state(g_i[0].sem_id, 2, LOCK);

    pthread_t thread_user1;
    pthread_t thread_user2;
    pthread_t master;

    pthread_create(&thread_user2, NULL, user2_turn, g_i);
	pthread_create(&thread_user1, NULL, user1_turn, g_i);
    pthread_create(&master, NULL, master_turn, g_i);

    pthread_join(thread_user2, NULL);
    pthread_join(thread_user1, NULL);
    pthread_join(master, NULL);

    if (g_i[0].s_arr[1] > g_i[0].s_arr[2])
		printf("User1 wins the game!\n");
	else
		printf("User2 wins the game!\n");
	
	return 0;
}