#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    printf("opening %s...\n", argv[1]);

    FILE* fp = fopen(argv[1], "r");

    if (!fp) {
        printf("Failed to open %s\n", argv[1]);
        return 1;
    }

    printf("successfully opened\n");

    return 0;
}
