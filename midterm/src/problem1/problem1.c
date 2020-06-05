#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* TODO */

char * 
read_exec (char * exe)
{
/* TODO*/
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
