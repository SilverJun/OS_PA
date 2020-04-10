#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    printf("Hello, my pid is: %d\n", getpid());
    printf("Waiting for...\n");
    getchar();
    return 0;
}
