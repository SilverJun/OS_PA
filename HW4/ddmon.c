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

//static pthread_mutex_t pipe_lock = PTHREAD_MUTEX_INITIALIZER;

void pipe_write(char* buf, int size) {
    //pthread_mutex_lock(&pipe_lock);
    int fd = open("./.ddtrace", O_WRONLY | O_APPEND);
    write(fd, buf, size);
    //pthread_mutex_unlock(&pipe_lock);
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    static __thread int n_lock = 0 ;
	n_lock += 1 ;

    int (*orig_mutex_lock)(pthread_mutex_t* mutex); 
    char* error;
    
    orig_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    if ((error = dlerror()) != 0x0) {
        fputs(error, stderr);
        exit(1);
    }

    if (n_lock == 1)
    {
        
        // pipe write
        char buf[256] = {0x0};
        sprintf(buf, "lock %lu %p\n", pthread_self(), mutex);
		void * arr[10] ;
		char ** stack ;

		size_t sz = backtrace(arr, 10);
		stack = backtrace_symbols(arr, sz);

        /**
         * [0] ./ddmon.so(pthread_mutex_lock+0x143) [...]
         * [1] ./abba() [...]  <--- this is deadlock location!
         * [2] /lib/x86_64-linux-gnu/libc.so.6(__libc_start_main+0xf0) [...]
         * [3] ./abba() [...]
         */
        printf("ddmon> mutex lock\n");
        printf("ddmon> stack[1]: %s\n", stack[1]);
        strcat(buf, stack[1]);
        strcat(buf, "\n");
        printf("ddmon> command: %s\n", buf);
        pipe_write(buf, strlen(buf));
    }

    // pthread_mutex_lock call
	int ret = orig_mutex_lock(mutex);
    n_lock -= 1;
	return ret;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    static __thread int n_unlock = 0 ;
	n_unlock += 1 ;

    int (*orig_mutex_unlock)(pthread_mutex_t* mutex); 
	char* error;

    orig_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
    if ((error = dlerror()) != 0x0) {
        fputs(error, stderr);
        exit(1);
    }

    // pthread_mutex_unlock call
    int ret = orig_mutex_unlock(mutex);

    if (n_unlock == 1) {
        // pipe write
        char buf[256] = {0x0};
        sprintf(buf, "unlock %lu %p\n", pthread_self(), mutex);
        printf("ddmon> mutex unlock\n");
        printf("ddmon> command: %s\n", buf);
        pipe_write(buf, strlen(buf));
    }

    n_unlock -= 1;
    return ret;
}
