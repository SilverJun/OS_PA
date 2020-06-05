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

#define N 10

int mutex_addr[N]; // mutex address saving array.

pthread_t t_table[N];
int t_size = 0;
typedef struct {
    int buf[N]; // it contains recent locked mutex node index.
    int top_idx;
} history_t;
history_t t_history[N];


int lock_graph[N][N];
int lock_graph_node_size = 0;

// for addr2line
char name[64] = {0x0};
char address[64] = {0x0};

void print_line_no() {
    char str[128] = {0x0};
    strcpy(str, "addr2line -e ");
    strcat(str, name);
    str[strlen(str)] = ' ';
    strcat(str, address);
    printf("cmd: %s\n", str);
    FILE* fp = popen(str, "r");
    if (fp == NULL) {
        printf("ddchck> addr2line open failed!\n");
        return;
    }
    fscanf(fp, "%s", str);
    printf("ddchck> %s\n", str);
    fclose(fp);
}

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

int find_cycle(int curr, int visited[N], int recur[N]) {
    visited[curr] = 1;
    recur[curr] = 1;

    for (int i = 0; i < N; i++)
    {
        if (lock_graph[curr][i] == 0) continue;
        
        if (!visited[i] && find_cycle(i, visited, recur)) return 1;
        else if (recur[i]) return 1;
    }
    recur[curr] = 0;
    return 0;
}

// graph에 노드 추가, 히스토리 보고 이전에 락 해둔거 있으면 그거랑 잇는 엣지 생성, 사이클 체크.
void lock(int t_idx, int l_idx) {
    history_t* this_thread = &t_history[t_idx];

    this_thread->buf[this_thread->top_idx++] = l_idx;

    if (this_thread->top_idx > 1) {
        printf("link edge!\n");
        lock_graph[this_thread->buf[this_thread->top_idx-2]][l_idx] = 1; // edge 추가.

        int visited[N];
        int recur[N];
        memset(visited, 0, sizeof(int)*N);
        memset(recur, 0, sizeof(int)*N);
        if (find_cycle(l_idx, visited, recur)) { // cycle 찾기.
            printf("ddchck> Deadlock detected at:\n");
            print_line_no();
            //abort();
        }
    }
}

void unlock(int t_idx, int l_idx) {
    printf("delete node!\n");
    for (int x = 0; x < N; x++) { // remove all edge which connected with popped mutex.
        lock_graph[l_idx][x] = 0;
        lock_graph[x][l_idx] = 0;
    }
    // pop thread history

    history_t* this_thread = &t_history[t_idx];
}

int push_mutex(int addr) {  
    printf("ddchck> create node!\n");  
    lock_graph_node_size++;
    for (int i = 0; i < N; i++) {
        if (mutex_addr[i] == 0) {
            mutex_addr[i] = addr;
            return i;
        }
    }
    return -1;
}

void pop_mutex(int t_idx, int addr) {
    lock_graph_node_size--;
    int l_idx = find_mutex_idx(addr);

    mutex_addr[l_idx] = 0;
    unlock(t_idx, l_idx);
}

void pipe_read(char* buf, int size) {
    //printf("ddchck> try to read fifo\n");
    int fd = open("./.ddtrace", O_RDONLY | O_SYNC);
    char* ptr = buf;

    while (1) {   // read one line from the pipe. 
        read(fd, ptr, 1);
        if (*ptr=='\0' || *ptr=='\n') { break; }
        ptr++;
    }
    close(fd);
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
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("file name should be provided.\n");
        exit(1);
    }
    
    // argv[1] is file name.
    strcpy(name, argv[1]);

    init();

    while (1)
    {
        // read pipe one line;
        char buf[2][128] = {0x0};

        pipe_read(buf[0], 128);
        pipe_read(buf[1], 128);

        // identify command
        char command[16] = {0x0};
        pthread_t tid = 0;
        int addr = 0;
        if (sscanf(buf[0], "%s %lu %p", command, &tid, &addr) == -1) break; // exit when read failed.
        printf("ddchck> received from pipe: %s %lu %p\n", command, tid, addr);

        char tmp[128] = {'\0'};
        int addr_int = 0;
        sscanf(buf[1], "%s [%x]", tmp, &addr_int); // extract stack info.
        //printf("%x\n", addr_int);
        sprintf(address, "0x%x", addr_int);
        //printf("%s, %s\n", name, address);

        int t_idx = find_thread_idx(tid);

        if (t_idx == -1) { // make new thread
            t_idx = t_size++;
            t_table[t_idx] = tid;
            printf("ddchck> new thread, current threads: %d\n", t_size);
        }
        printf("ddchck> t_idx: %d\n", t_idx);
        // do that command
        if (strcmp("lock", command) == 0){ // pthread mutex lock
            printf("ddchck> lock\n");

            int l_idx = find_mutex_idx(addr);
            printf("ddchck> l_idx: %d\n", l_idx);
            if (l_idx == -1) {
                l_idx = push_mutex(addr);
            }
            lock(t_idx, l_idx);
        }
        else if (strcmp("unlock", command) == 0) { // pthread mutex unlock
            printf("ddchck> unlock\n");
            pop_mutex(t_idx, addr);
        }
        else {
            printf("Undefined command received.\n");
        }

        // repeat until program exit.
    }

    return 0;
}
