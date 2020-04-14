#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>


#include "msg.h"

typedef struct _tagMsg {
    short type;
    uid_t uid;
    char fname[FNAME_SIZE];
} JerryMsg;

JerryMsg msg;

void usage();
int parsing_message(int argc, char* argv[]);
void pass2mousehole();
uid_t getUserIdByName(const char *name)
{
    struct passwd *pwd = getpwnam(name); /* don't free, see getpwnam() for details */
    if(pwd == NULL) {
        printf("Failed to get userId from username : %s\n", name);
        exit(1);
    } 
    return pwd->pw_uid;
}

int main (int argc, char* argv[]) {
    //printf("argc: %d\n", argc);
    if (argc < 2 || parsing_message(argc, argv)) { usage(); exit(1); }

    // pass message to mousehole.c through proc filesystem.
    pass2mousehole();

    //exit with 0
    exit(0);
}

void usage() {
    printf("<usage>\n");
    // -bf uname fname
    printf("\t-bf <uname> <fname>\n");
    printf("\t\tthis command blocks file open by given user\n\n");

    // -uf
    printf("\t-uf\n");
    printf("\t\tundo blocking file open\n\n");

    // -pk
    printf("\t-pk <uname>\n");
    printf("\t\tthis command protects open by user name\n\n");

    // -uk
    printf("\t-uk\n");
    printf("\t\tundo protecting kill\n\n");
}

int parsing_message(int argc, char* argv[]) {
    char* msg_table[] = {
        "-bf",
        "-pk",
        "-uf",
        "-uk",
    };

    //printf("%s\n", argv[1]);
    int i;
    for (i = 0; i < MSG_COUNT; ++i) {
        if (strcmp(argv[1], msg_table[i]) == 0) {
            msg.type = (short)i;
            break;
        }
    }
    if (i == MSG_COUNT) return 1;
    //printf("%d\n", msg.type);
    
    switch (msg.type)
    {
        case BLOCK_FILE:
            if (argc < 4) return 1;
            // file name - argv[4];
            strcpy(msg.fname, argv[3]);
        case PREVENT_KILL:
            if (argc < 3) return 1;
            // user name - argv[3];
            msg.uid = getUserIdByName(argv[2]);
            break;
        case UNDO_FILE:
        case UNDO_KILL:
            break;
        default: puts("parsing_message: msg.type error!"); exit(1);
    }

    return 0;
}

void pass2mousehole() {

    int fd = open("/proc/mousehole", O_WRONLY);
    if (fd < 0) {
        printf("/proc/mousehole open error!\n");
        exit(1);
    }

    char buf[MSG_SIZE] = {0x0};
    int size = 0;

    switch (msg.type)
    {
        case BLOCK_FILE:
            size = sprintf(buf, "%d %d %s", msg.type, msg.uid, msg.fname);
            break;
        case PREVENT_KILL:
            size = sprintf(buf, "%d %d", msg.type, msg.uid);
            break;
        case UNDO_FILE:
        case UNDO_KILL:
            size = sprintf(buf, "%d", msg.type);
            break;
        default: puts("pass2mousehole: msg.type error!"); exit(1);
    }
    
    if (size < 0) {
        puts("sprintf error");
        exit(1);
    }

    printf("Message: %d-%s-%d\n", fd, buf, size);

    ssize_t ret = write(fd, buf, size);

    printf("write: %d\n", ret);
    puts("Jerry: Successfully done.");
    close(fd);
}
