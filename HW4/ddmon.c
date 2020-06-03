#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


void pipe_write(char* buf, int size) {
    int fd = open(".ddtrace", O_WRONLY | O_SYNC);
    write(fd, buf, size);
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    printf("ddmon> mutex lock\n");
    int (*orig_mutex_lock)(pthread_mutex_t* mutex); 
	char* error;
	
	orig_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if ((error = dlerror()) != 0x0) {
	    fputs(error, stderr);
		exit(1);
    }
    
    // pthread_mutex_lock call
	int ret = orig_mutex_lock(mutex);
    printf("ddmon> mutex lock done with %d.\n", ret);

    // pipe write
    char buf[128] = {0x0};
    sprintf(buf, "lock %lu %p\n", pthread_self(), mutex);
    pipe_write(buf, strlen(buf));

	return ret;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    printf("ddmon> mutex lock\n");
    int (*orig_mutex_unlock)(pthread_mutex_t* mutex); 
	char* error;
	
	orig_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	if ((error = dlerror()) != 0x0) {
	    fputs(error, stderr);
		exit(1);
    }

    // pthread_mutex_unlock call
	int ret = orig_mutex_unlock(mutex);
    printf("ddmon> mutex unlock done with %d.\n", ret);

    // pipe write
    char buf[128] = {0x0};
    sprintf(buf, "unlock %lu %p\n", pthread_self(), mutex);
    pipe_write(buf, strlen(buf));

	return ret;
}

