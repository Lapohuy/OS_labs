#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include <unistd.h>

void* print_message_func1()
{
	for(int i = 1; i < 11; i++)
		printf("Hello Thread(%i)\n", i), sleep(1);
	return NULL;
}

void* print_message_func2()
{
	for (int i = 1; i < 13; i++)
		printf("This is iteration %i\n", i), sleep(2);
	return NULL;
}

int main(void)
{
	pthread_t thrd1, thrd2;
	
	int res1 = pthread_create(&thrd1, NULL, print_message_func1, NULL);
	int res2 = pthread_create(&thrd2, NULL, print_message_func2, NULL);
	
	int iret1, iret2;
	
	pthread_join(thrd1, (void**) &iret1);
	pthread_join(thrd2, (void**) &iret2);
	
	return 0;
}
