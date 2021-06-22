all:

	gcc client.c -o final_client.o -c
	gcc server.c -o final_server.o -c
	gcc utility.c -o utility.o -c

	gcc final_server.o utility.o -lpthread -o server
	gcc final_client.o utility.o -lpthread -o client

	rm final_client.o final_server.o utility.o