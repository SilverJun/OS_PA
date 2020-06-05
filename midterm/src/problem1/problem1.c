#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
/* TODO */

int pipes[2] ;

char * 
read_exec (char * exe)
{
	pid_t child_pid ;
	
	if (pipe(pipes) != 0) {
		perror("Error") ;
		exit(1) ;
	}
	
	child_pid = fork() ;
	if (child_pid == 0) {
		close(pipes[0]);
		dup2(pipes[1], 1 /* STDOUT*/) ;
		execl(exe, exe, 0x0);
		exit(0);
	}
	else {
		wait(0x0);
		close(pipes[1]);
		char* buf = (char*)malloc(sizeof(char) * 2048);
		// get string from pipe and print that.
		ssize_t s ;
		while ((s = read(pipes[0], buf, 2047)) > 0) {
			buf[s + 1] = 0x0 ;
			//printf(">%s\n", buf) ;
		}
		return buf;
	}
}

int 
main (int argc, char ** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Wrong number of arguments\n") ;
		exit(1) ;
	}

	char * s ;
	s = read_exec(argv[1]) ;
	printf("\"%s\"\n", s) ;
	free(s) ;
	exit(0) ;
}
