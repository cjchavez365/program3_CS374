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

/* parseCommand MAIN FUNCTIONALITIES ARE TO TAKE THE USERS INPUT
 * AND CREATE COMMANDS, ARGUMENTS, INPUT/OUTPUT REDIRECTION FILES, AND 
  * BACKGROUND FLAGS - AS OUTLINED IN THE ASSIGNMENT DESCRIPTION */
void parseCommand (char* input, struct Command* cmd) {
    int i; 
    int argc = 0;
    char* token;

    // Initialize command values before parsing
    cmd->inputFile = NULL;
    cmd->outputFile = NULL;
    cmd->background = 0;

    // Sets all argument pointers to NULL. Uses for loop since
    // there is a quantifiable amount of arguments
    for (i = 0; i < MAX_ARGS + 1; i++) {
        cmd->args[i] = NULL;
    }

    // Breaks user input into tokens separated by spaces
    token = strtok(input, " ");

    while (token != NULL) {
        // Considers input redirection
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            cmd->inputFile = token;
        }
        // Considers output redirection
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            cmd->outputFile = token;
        }
        // Normal command arguments
        else {
            cmd->args[argc] = token;
            argc++;
        }
        token = strtok(NULL, " ");
    }

    // Checks if the command should run in the background
    if (argc > 0 && strcmp(cmd->args[argc - 1], "&") == 0) {
        cmd->background = 1;
        cmd->args[argc - 1] = NULL;
    }

    // Ignores background mode if the shell is in foreground-only mode
    if (foregroundOnlyMode == 1) {
        cmd->background = 0;
    }
}

int runBuiltIn (struct Command* cmd, int lastStatus, int lastSignal) {
    if (strcmp(cmd->args[0], "exit") == 0) {
        //killBackgroundProcess();
        exit(0);
    }
    if (strcmp(cmd->args[0], "cd") == 0) {
        if (cmd->args[1] == NULL) {
            chdir(getenv("HOME"));
        }
        else {
            chdir(cmd->args[1]);
        }
        return 1;
    }
    if (strcmp(cmd->args[0], "status") == 0) {
        if (lastSignal == 0) {
            printf("exit value %d\n", lastStatus);
        }
        else {
            printf("terminated by signal %d\n", lastSignal);
        }
        fflush(stdout);
        return 1;
    }
    return 0;
}

/* THERE ARE SEVERAL FUNCTIONALITIES WITHIN THE MAIN FUNCTION:
 * The while loop continuously prompts and processes input from user and follow the required format of ": "
 * 1st implementation: accounts for blank entries and comments indicated by # */
int main (){
    char input[MAX_LINE + 1];
    char expandedInput[MAX_LINE * 2];
    int lastStatus = 0;
    int lastSignal = 0;

    /* This is the while loop that allows for continuous prompting
     * a while loop was chosen so it would only end if user decides
     * if another loop was chosen then it would've been quantifiable
     * and not constant. */
    while (1) {
        printf(": ");
        fflush(stdout);
        fgets(input, MAX_LINE, stdin); // Gets users' input

        input[strcspn(input, "\n")] = '\0';

        // Checks if input is either blank or a comment
        if (input[0] == '\0' || input[0] == '#') {
            continue;
        }

        struct Command cmd;

        strcpy(expandedInput, input);
        parseCommand(expandedInput, &cmd);

        if (cmd.args [0] == NULL) {
            continue;
        }

        runBuiltIn(&cmd, lastStatus, lastSignal);
    }
    return 0;
}

