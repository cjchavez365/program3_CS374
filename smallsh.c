/* LIBRARIES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

/* MACROS USED THROUGHOUT THE PROGRAM
 * DEFINED HERE TO SIMPLIFY FILE*/
#define MAX_LINE 2048 // "Shell must support command lines of max length of 2048 char"
#define MAX_ARGS 512 // "Support a max of 512 arguments"
#define MAX_BG 100 // Used for the array implementation

int foregroundOnlyMode = 0;

pid_t bgPids[MAX_BG];
int bgCount = 0;

struct Command {
    char* args[MAX_ARGS + 1];
    char* inputFile;
    char* outputFile;
    int background;
};

int main (){
    char input[MAX_LINE + 1];

    while (1) {
        printf(": ");
        fflush(stdout);
        fgets(input, MAX_LINE, stdin);

        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0' || input[0] == '#') {
            continue;
        }
    }
    return 0;
}

