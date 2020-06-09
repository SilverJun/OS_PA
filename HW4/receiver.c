#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int 
main ()
{
	int fd = open(".ddtrace", O_RDONLY) ;

	while (1) {
		char s[128] = {0x0};
		char* ptr = s;

		int len ;
		while (1) {   // read one line from the pipe. 
			read(fd, ptr, 1);
			if (*ptr=='\0' || *ptr=='\n') { break; }
			ptr++;
		}
		if (strlen(s) != 0) {
			printf("%s", s);
			printf("------\n");
		}
	}
	close(fd) ;
	return 0 ;
}
