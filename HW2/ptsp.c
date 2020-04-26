#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <limits.h>
#include <string.h>

// pipe
int pipes[2];

// tsp
int** m;
int* path;
int* best_path;
int* checked;
int N = 0;
int dist = 0;
int min_dist = INT_MAX;

unsigned long long checked_count = 0;

// parallel
int process_count = 0; // maximum child process
int current_active = 0;
pid_t children[12] = {0};
int is_term = 0;

void init(char* filename) { // memory allocation and initialization value.
    FILE* fp = fopen(filename, "r");
    int temp = 0;
    checked = (int*)malloc(sizeof(int)*N);
    memset(checked, 0, sizeof(int)*N);
    path = (int*)malloc(sizeof(int)*N);
    memset(path, 0, sizeof(int)*N);
    best_path = (int*)malloc(sizeof(int)*N);
    memset(best_path, 0, sizeof(int)*N);

    m = (int**)malloc(sizeof(int*)*N);
    for (int i = 0; i < N; i++) {
        m[i] = (int*)malloc(sizeof(int)*N);
		for (int j = 0; j < N; j++) {
			fscanf(fp, "%d", &temp);
			m[i][j] = temp;
		}
	}
	fclose(fp);
}

void release() {
    for (int i = 0; i < N; i++) {
        free(m[i]);
    }
    free(m);
    free(best_path);
    free(path);
    free(checked);
}

void write_pipe() {
    char buf[128] = {0};

    if (min_dist != INT_MAX) {  // pipe: <count>,<dist>,<best path>
        char path_string[128] = {0};
        // path string build
        for (int i = 0; i < N; ++i) {
            char temp[8] = {0};
            sprintf(temp, "%d ", best_path[i]);
            strcat(path_string, temp);
        }
        strcat(path_string, "0");

        sprintf(buf, "%llu,%d,%s\n", checked_count, min_dist, path_string);
    }
    else {  // case of visit 0 path.
        sprintf(buf, "0,0,0\n");
    }
    //printf("write_pipe: %s", buf);
    write(pipes[1], buf, strlen(buf));
    // printf("child write pipe done\n");
}

void read_pipe() {
    char buf[128] = {0};
    char* ptr = buf;

    // printf("try to read pipe line\n");
    while (1) {   // read one line from the pipe. 
        read(pipes[0], ptr, 1);
        if (*ptr++=='\n') { ptr++; break; }
    }
    //printf("read_pipe: %s", buf);
    
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

void print_result() {
    printf("The best route: <");
    for (int i = 0; i < N; i++) {
        printf("%d ", best_path[i]);
    }
    printf("0>\n");
    
    printf("The best minimum distance is %d\n", min_dist);
    printf("Total checked count is %llu\n", checked_count);
}

void sigchld_handler(int sig) { // When the child process found the best route.
    pid_t pid;
    int status;

    while(!is_term) {
        if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            //printf("receive %d child\n", pid);

            for (int i = 0; i < process_count; i++) {
                if (children[i] == pid) {
                    children[i] = 0;
                    break;
                }
            }
            
            read_pipe();

            current_active--;
            break;  
        }
    }
}

void parent_sigint_handler(int sig) {
    // printf("parent sigint start\n");
    is_term = 1;
    int status;
    pid_t pid;
    // printf("active child: %d\n", current_active);
    while(current_active != 0) {
        while(1) {
            if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                // printf("receive %d child\n", pid);
                read_pipe();
                current_active--;
                break;  
            }
        }
    }
    // printf("parent sigint end\n");

    print_result();

    release();
    exit(0);
}

void child_sigint_handler(int sig) {
    // send result through pipe
    // printf("child sigint\n");

    write_pipe();
    
    exit(0);
}


// visit city
void visit(int i) {
    //printf("visit %d\n", i);
    if (i == N) {
        checked_count++;
        dist += m[path[i-1]][0];
        
        // update if this result is shortest
        if (dist < min_dist) {
            min_dist = dist;
            for (int k = 0; k < N; k++) {
                best_path[k] = path[k];
            }
        }
        //
        dist -= m[path[i-1]][0];
        return;
    }
    for (int j = 0; j < N; j++) {
        //printf("visit j: %d\n", j);
        //printf("checked[j]: %d", checked[j]);
        if (checked[j]==1) continue;
        checked[j] = 1;
        path[i] = j;

        dist += m[path[i-1]][j];

        if (N - 12 == i+1) { // check if need to spawn child.
            //printf("need to spawn child\n");
            while (current_active >= process_count) { usleep(10); };
            
            pid_t child;
            //printf("fork!\n");
            if (child = fork()) { // parent
                for (int k = 0; k < process_count; k++) {  // add child pid to empty slot.
                    if (!children[k]) {
                        children[k] = child;
                        break;
                    }
                }
                // printf("child forked!\n");
                current_active++;
            }
            else { // child case
                checked_count = 0;
                signal(SIGINT, child_sigint_handler); // change sigint to child_sigint_handler to write pipe.
                visit(i+1);
                // printf("child done\n");
                write_pipe();
                release();
                exit(0);
            }
        }
        else {
            visit(i+1);
        } // parent case before generating child.

        checked[j] = 0;
        dist -= m[path[i-1]][j];
    }
}

int main(int argc, char* argv[]) {
    sscanf(argv[2], "%d", &process_count);
	FILE* fp = fopen(argv[1], "r");
    //sscanf(argv[1], "gr%d.tsp", &len); // this is some trick

    if (!fp) {
        printf("file open error\n");
        exit(1);
    }
    
    while (!feof(fp)) { // check len
        char buf[256]={0};
        fgets(buf, 256, fp);
        if (strlen(buf)) N++;
    }
    fclose(fp);

    init(argv[1]);

    printf("parent pid: %d\n", getpid());
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, parent_sigint_handler);

    if (pipe(pipes) != 0) {
        perror("Failed to open pipe");
        exit(1);
    }

    // go!
    path[0] = 0;
    checked[0] = 1;
    visit(1);
    checked[0] = 0;

    printf("all job is done. wait for end of children.\n");
    while (current_active != 0) usleep(100); // wait until finish all child processes.

    print_result();

    return 0;
}
