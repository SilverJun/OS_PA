#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

extern int errno ;

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    printf("opening %s...\n", argv[1]);

    FILE* fp = fopen(argv[1], "r");

    if (fp==NULL) {
        printf("Failed to open %s\n", argv[1]);
        printf("Value of errno: %d\n", errno);
        printf("Error opening file: %s\n", strerror( errno ));
        return 1;
    }

    printf("successfully opened\n");

    return 0;
}
