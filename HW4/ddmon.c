#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>


void pipe_write(char* buf, int size) {
    int fd = open(".ddtrace", O_WRONLY | O_SYNC);
    
    #ifdef DEBUG
    printf("write_pipe: %s", buf);
    #endif
    write(fd, buf, size);
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    printf("ddmon> mutex init\n");
    int (*orig_mutex_init)(size_t size); 
	char* error;
	
	orig_mutex_init = dlsym(RTLD_NEXT, "pthread_mutex_init");
	if ((error = dlerror()) != 0x0) {
	    fputs(error, stderr);
		exit(1);
    }

	int ret = orig_mutex_init(mutex, attr);

    if (!ret) {
        // pipe write
        sprintf(buf, "0,0,0\n");
        pipe_write();
    }
    printf("ddmon> mutex init done\n");

	return ret;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {

}

int pthread_mutex_lock(pthread_mutex_t* mutex) {

}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {

}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {

}

