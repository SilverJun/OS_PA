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
#include <signal.h>

const int N = 10;

int mutex_addr[N]; // mutex address saving array.

pthread_t t_table[N];
int t_size = 0;
typedef struct {
    int buf[N]; // it contains recent locked mutex node index.
    int top_idx;
} stack_t;
stack_t t_history[N];


int lock_graph[N][N];
int lock_graph_node_size = 0;

int find_thread_idx(pthread_t tid) {
    for (int i = 0; i < t_size; i++)
        if (t_table[i] == tid) return i;
    
    return -1;
}

int find_mutex_idx(int addr) {
    for (int i = 0; i < N; i++)
        if (mutex_addr[i] == addr) return i;
    
    return -1;
}

int push_mutex(int t_idx, int addr) {
    for (int i = 0; i < N; i++) {
        if (mutex_addr[i] == 0) {
            mutex_addr[i] = addr;
            lock(t_idx, i);
            lock_graph_node_size++;
            return i;
        }
    }
    return -1;
}

void pop_mutex(int t_idx, int addr) {
    int l_idx = find_mutex_idx(addr);

    mutex_addr[l_idx] = 0;
    unlock(t_idx, l_idx);

    lock_graph_node_size--;
}

int find_cycle(int visited[N], int path[N]) {
    
}

// graph에 노드 추가, 히스토리 보고 이전에 락 해둔거 있으면 그거랑 잇는 엣지 생성, 사이클 체크.
void lock(int t_idx, int l_idx) {

}

void unlock(int t_idx, int l_idx) {
    for (int x = 0; x < lock_graph_node_size; x++) { // remove all edge which connected with popped mutex.
        lock_graph[x][l_idx] = 0;
    }
    // pop thread history
}

void pipe_read(char* buf, int size) {
    int fd = open(".ddtrace", O_WRONLY | O_SYNC);
    char* ptr = buf;

    while (1) {   // read one line from the pipe. 
        read(fd, ptr, 1);
        if (*ptr++=='\n') { ptr++; break; }
    }
}

int sigint_handler(int sig) {
    // clear .ddtrace file.
    
    exit(0);
}

void init() {
    // 배열 초기화.
    for (int i = 0; i < N; i++)
    {
        memset(lock_graph[i], 0, sizeof(int) * N);
        t_history[i].top_idx = 0;
        memset(t_history[i].buf, 0, sizeof(int) * N);
    }

    memset(t_table, 0, sizeof(pthread_t) * N);
    memset(mutex_addr, 0, sizeof(int) * N);
    
    signal(SIGINT, sigint_handler);
}

int main(int argc, char* argv[]) {

    init();

    while (1)
    {
        // read pipe one line;
        char buf[128] = {0x0};
        pipe_read(buf, 128);
        // identify command
        char command[16] = {0x0};
        pthread_t tid;
        int addr;
        sscanf(buf, "%s %lu %p", command, &tid, &addr);

        printf("ddchck> received from pipe: %s %lu %p", command, tid, addr);

        int t_idx = find_thread_idx(tid);

        if (t_idx == -1) { // make new thread
            t_idx = t_size++;
            t_table[t_idx] = tid;
        }

        // do that command
        if (strcmp("lock", command) == 0){ // pthread mutex lock
            push_mutex(t_idx, addr);
        }
        else if (strcmp("unlock", command) == 0) { // pthread mutex unlock
            pop_mutex(t_idx, addr);
        }
        else {
            printf("Undefined command received.\n");
        }

        // repeat until program exit.
    }

    return 0;
}
