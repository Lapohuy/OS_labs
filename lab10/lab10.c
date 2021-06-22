#include "stdio.h"
#include "stdlib.h"
#include "signal.h"

unsigned SIGINT_count;
unsigned SIGUSR1_count;
unsigned SIGUSR2_count;

void print_signals_count()
{
	printf("SIGINT = %i\n", SIGINT_count);
	printf("SIGUSR1 = %i\n", SIGUSR1_count);
	printf("SIGUSR2 = %i\n", SIGUSR2_count);
}

void signal_handler(int nsig)
{
	switch (nsig)
	{
		case SIGINT:  SIGINT_count++;  break;
		case SIGUSR1: SIGUSR1_count++; break;
		case SIGUSR2: SIGUSR2_count++; break;
		default: break;
	}
}

int main(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);

	while (1)
	{
		system("date");
		if (SIGINT_count < 5)
			print_signals_count();
		else
		{
			printf("========== RESULT ==========\n");
			print_signals_count();
			printf("============================\n");
			break;
		}

		sleep(1);
	}

	return 0;
}