#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* TODO: you can freely edit this file */

pthread_t recv, send;
	
void sigint_handler(int sig) {
	pthread_cancel(recv);
	pthread_cancel(send);
}

void* receiver(void* ptr) {
	char * r_fifo = (char*)ptr ;

	while (1)
	{
		int ch_read = open(r_fifo, O_RDONLY | O_NONBLOCK) ;
		//printf("receiver> %d\n", ch_read);
		char buf[256] = {0x0};

		while (read(ch_read, buf, 256) < 1);

		printf("> %s\n", buf);

		pthread_testcancel();
		close(ch_read);
	}
	
	return 0;
}

void* sender(void* ptr) {
	char * w_fifo = (char*)ptr ;
	printf("%s\n", w_fifo);
	while(1) {
		int ch_write = 0;
		while ((ch_write = open(w_fifo, O_WRONLY | O_NONBLOCK)) < 0);
		//printf("sender> %d\n", ch_write);
		char buf[256] = {0x0};

		fgets(buf, 256, stdin);
		buf[strlen(buf)-1] = '\0';

		write(ch_write, buf, 256);
		pthread_testcancel();
		close(ch_write);
	}
	return 0;
}

int
main (int argc, char ** argv)
{
	if (argc != 3) {
		fprintf(stderr, "Wrong number of arguments\n") ;
		exit(1) ;
	}

	char * r_fifo = argv[1] ;
	char * w_fifo = argv[2] ;

	//printf("%s, %s\n", r_fifo, w_fifo);

	signal(SIGINT, sigint_handler);

	pthread_create(&recv, 0x0, receiver, r_fifo);
	pthread_create(&send, 0x0, sender, w_fifo);

	pthread_join(recv, 0x0);
	pthread_join(send, 0x0);

	printf("Good bye\n");

	return 0;
}
