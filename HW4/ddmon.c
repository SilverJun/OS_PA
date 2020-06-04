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
    int fd = open("./.ddtrace", O_WRONLY | O_SYNC);
    write(fd, buf, size);
}


// int pthread_create(pthread_t * __newthread,
// 			   const pthread_attr_t * __attr,
// 			   void *(*__start_routine) (void *),
// 			   void * __arg) {
//     int (*orig_func)(pthread_t * __newthread,
// 			   const pthread_attr_t * __attr,
// 			   void *(*__start_routine) (void *),
// 			   void * __arg);
// 	char* error;
	
// 	orig_func = dlsym(RTLD_NEXT, "pthread_create");
// 	if ((error = dlerror()) != 0x0) {
// 	    fputs(error, stderr);
// 		exit(1);
//     }

// 	return orig_func(__newthread, __attr, __start_routine, __arg);
// }

// int pthread_join(pthread_t __th, void **__thread_return) {
//     int (*orig_func)(pthread_t __th, void **__thread_return); 
// 	char* error;
	
// 	orig_func = dlsym(RTLD_NEXT, "pthread_join");
// 	if ((error = dlerror()) != 0x0) {
// 	    fputs(error, stderr);
// 		exit(1);
//     }
// 	return orig_func(__th, __thread_return);
// }

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    static __thread int n_call = 0 ; //https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Thread-Local.html
	n_call += 1 ;

    int (*orig_mutex_lock)(pthread_mutex_t* mutex); 
    char* error;
    
    orig_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    if ((error = dlerror()) != 0x0) {
        fputs(error, stderr);
        exit(1);
    }

    if (n_call == 1)
    {
        //printf("ddmon> mutex lock\n");
        
        // pipe write
        char buf[128] = {0x0};
        sprintf(buf, "lock %lu %p\n", pthread_self(), mutex);
        //printf("ddmon> command: %s\n", buf);
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
        //printf("ddmon> stack[1]: %s\n", stack[1]);
        strcat(buf, stack[1]);
        strcat(buf, "\n");
        //printf("ddmon> command: %s\n", buf);
        pipe_write(buf, strlen(buf));
    }

    // pthread_mutex_lock call
	int ret = orig_mutex_lock(mutex);
    n_call -= 1;
	return ret;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    static __thread int n_call = 0 ; //https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Thread-Local.html
	n_call += 1 ;

    int (*orig_mutex_unlock)(pthread_mutex_t* mutex); 
	char* error;

    orig_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
    if ((error = dlerror()) != 0x0) {
        fputs(error, stderr);
        exit(1);
    }

    if (n_call == 1) {
        //printf("ddmon> mutex unlock\n");
        
        // pthread_mutex_unlock call
        int ret = orig_mutex_unlock(mutex);

        // pipe write
        char buf[128] = {0x0};
        sprintf(buf, "unlock %lu %p\n", pthread_self(), mutex);
        //printf("ddmon> command: %s\n", buf);
        pipe_write(buf, strlen(buf));

        n_call -= 1;
	    return ret;
    }

    n_call -= 1;
    return orig_mutex_unlock(mutex);
}
