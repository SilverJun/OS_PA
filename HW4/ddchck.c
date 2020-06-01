#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>


void pipe_read(char* buf, int size) {
    int fd = open(".ddtrace", O_WRONLY | O_SYNC);
    char buf[128] = {0};
    char* ptr = buf;

    // printf("try to read pipe line\n");
    while (1) {   // read one line from the pipe. 
        read(pipes[0], ptr, 1);
        if (*ptr++=='\n') { ptr++; break; }
    }
    
    #ifdef DEBUG
    printf("read_pipe: %s", buf);
    #endif 

    // parsing
    ptr = strtok(buf, ",");

    // count
    unsigned long long local_count = 0;
    sscanf(ptr, "%llu", &local_count);
    ptr = strtok(NULL, ",");

    if (local_count == 0) return; // if child can't visit any path, then local_count is 0. so I can just skip this result. 
    checked_count += local_count;

    // dist
    int local_dist;
    sscanf(ptr, "%d", &local_dist);
    ptr = strtok(NULL, ",");

    // update data if it needs.
    if (local_dist < min_dist) {
        min_dist = local_dist;
        // path
        char* path_ptr = ptr;
        ptr = strtok(path_ptr, " ");
        for (int i = 0; i < N; ++i) {
            sscanf(ptr, "%d", &best_path[i]);
            ptr = strtok(NULL, " ");
        }
    }
}


int main(int argc, char* argv[]) {


    return 0;
}