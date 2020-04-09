#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void usage();

int main (int argc, char* argv[]) {
    if (argc < 2) { usage(); exit(1); }

    // parsing cli message
    
    // check validation

    // pass message to mousehole.c through proc filesystem.

    //exit with 0
    exit(0);
}

void usage() {
    printf("<usage>\n");
    // -bf uname fname
    printf("-bf <uname> <fname>\n");
    printf("\t\tthis command blocks opening file by given user.\n");

    // -uf

    // -bk

    // -uk
    
}
