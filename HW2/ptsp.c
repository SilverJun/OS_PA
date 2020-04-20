#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>

// pipe
int pipes[2];

// tsp
int global_count = 0;
int maximum_cases = 0;

int** m;
int* path;
int N = 0;
int checked = 0;

// parallel
int process_count = 0;
int current_active = 0;
pid_t children[12];

void sigchld_handler(int sig) // When the child process found the best route.
{
    // pid_t child;
    // int exitcode;
    // child = wait(&exitcode);

    // printf("> child process %d is terminated with exitcode %d\n", child, WEXITSTATUS(exitcode));
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("receive %d child\n", pid);
        // something happened with child 'pid', do something about it...
        // Details are in 'status', see waitpid() manpage
    }

    // pipe read.

    // if check all possible cases, send term signal.
}

void child_main(int i) {
    pid_t id = getpid();
    printf("this child pid: %d, %dth child.\n", id, i);
    while (1);
    
    exit(0);
}

void parent_sigint_handler(int sig) {
    printf("parent sigint\n");
}

void child_sigint_handler(int sig) {
    // send best route and length through pipe
    printf("child sigint\n");
    char buf[128] = {0};
    sprintf(buf, "%d\n", getpid());
    write(pipes[1], buf, strlen(buf));
    exit(0);
}

pid_t spawn_child(int i) {
    pid_t child;
    if (child = fork()) {
        return child;
    }
    else {
        signal(SIGINT, child_sigint_handler);
        child_main(i);
    }
}

void init(char* filename) {
    FILE* fp = fopen(filename, "r");
    int temp = 0;
    path = (int*)malloc(sizeof(int)*N);
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

}

void print_result() {
    printf("The best route: <");
    for (int i = 0; i < N; i++)
    {
        printf("%d ", path[i]);
    }
    
    printf("%s %d");
}

int length = 0;

void visit(int i) {
    if (i-12 < 0) {
        if (fork()==0) {
            visit(i-1);
            exit(0);
        }
    }
    
}

int main(int argc, char* argv[])
{
    process_count = argv[2] - '0';
	FILE* fp = fopen(argv[1], "r");
    //sscanf(argv[1], "gr%d.tsp", &len);
    
    while (!feof(fp)) // check len
    {
        char buf[256]={0};
        fgets(buf, 256, fp);
        if (strlen(buf)) N++;
    }
    fclose(fp);


    printf("parent pid: %d\n", getpid());
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, parent_sigint_handler);

    if (pipe(pipes) != 0) {
        perror("Failed to open pipe");
        exit(1);
    }

    for (int i = 0; i < 10; ++i) {
        spawn_child(i);
    }
    
    while (1) sleep(1); // wait until finished.

    return 0;
}
