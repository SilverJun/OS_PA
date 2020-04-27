#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <limits.h>
#include <string.h>

int pipes[2];
int process_count = 0; // maximum child process
int current_active = 0;
pid_t children[12] = {0};
int is_term = 0;

void write_pipe() {
    char buf[128] = {0};

    pid_t id = getpid();
    sprintf(buf, "this child pid: %d child.\n", id);
    
    //printf("write_pipe: %s", buf);
    write(pipes[1], buf, strlen(buf));
}

void read_pipe() {
    char buf[128] = {0};
    char* ptr = buf;

    // printf("try to read pipe line\n");
    while (1) {   // read one line from the pipe. 
        read(pipes[0], ptr, 1);
        if (*ptr++=='\n') { ptr++; break; }
    }
    printf("read_pipe: %s", buf);
    
}

void sigchld_handler(int sig) // When the child process found the best route.
{
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

void child_main(int i) {
    write_pipe();
    exit(0);
}

void parent_sigint_handler(int sig) {
    // printf("parent sigint start\n");
    is_term = 1;
    int status;
    pid_t pid;
    printf("active child: %d\n", current_active);
    while(current_active != 0) {
        while(1) {
            if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                printf("receive %d child\n", pid);
                read_pipe();
                current_active--;
                break;  
            }
        }
    }
    exit(0);
}

void child_sigint_handler(int sig) {
    // send result through pipe
    // printf("child sigint\n");

    write_pipe();
    
    exit(0);
}

pid_t spawn_child(int i) {
    pid_t child;
    if (child = fork()) {
        for (int i = 0; i < process_count; i++) {
            if (children[i] == 0) {
                children[i] = child;
                break;
            }
        }
        printf("process %d has been spawned\n", child);
        current_active++;
        return child;
    }
    else {
        signal(SIGINT, child_sigint_handler);
        child_main(i);
    }
}

int main()
{
    printf("parent pid: %d\n", getpid());
    //signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, parent_sigint_handler);
    signal(SIGINT, parent_sigint_handler);
    process_count = 10;
    if (pipe(pipes) != 0) {
        perror("Failed to open pipe");
        exit(1);
    }

    for (int i = 0; i < 10; ++i) {
        spawn_child(i);
    }
    
    while (current_active != 0) usleep(10); // wait until finished.

    return 0;
}
