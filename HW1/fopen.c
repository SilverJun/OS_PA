#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    printf("opening %s...\n", argv[1]);

    int fd = open(argv[1], O_RDONLY);

    if (fd < 0) {
        printf("Failed to open %s\n", argv[1]);
        printf("Value of errno: %d\n", fd);
        printf("Error opening file: %s\n", strerror( -fd ));
        return 1;
    }

    printf("successfully opened\n");
    close(fd);
    return 0;
}
