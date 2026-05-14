/* LIBRARIES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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

/* expandPID MAIN FUNCTIONALITY IS TO REPLACE $$ FROM USER INPUT
 * WITH THE smallsh PID */
void expandPID (char* input, char* output) {
    char pidStr[20];
    sprintf(pidStr, "%d", getpid());

    output[0] = '\0';

    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '$' && input[i + 1] == '$') {
            strcat(output, pidStr);
            i++;
        }
        else {
            int len = strlen(output);
            output[len] = input[i];
            output[len + 1] = '\0';
        }
    }
}

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

void setupInput(struct Command* cmd) {
    int inputFD;

    if (cmd->inputFile != NULL) {
        inputFD = open(cmd->inputFile, O_RDONLY);

        if (inputFD == -1) {
            printf("cannot open %s for input\n", cmd->inputFile);
            fflush(stdout);
            exit(1);
        }

        dup2(inputFD, STDIN_FILENO);
        close(inputFD);
    }
    else if (cmd->background == 1) {
        inputFD = open("/dev/null", O_RDONLY);
        dup2(inputFD, STDIN_FILENO);
        close(inputFD);
    }
}

void setupOutput (struct Command* cmd) {
    int outputFD;

    if (cmd->outputFile != NULL) {
        outputFD = open(cmd->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (outputFD == -1) {
            printf("cannot open %s for output\n", cmd->outputFile);
            fflush(stdout);
            exit(1);
        }

        dup2(outputFD, STDOUT_FILENO);
        close(outputFD);
    }
    else if (cmd->background == 1) {
        outputFD = open("/dev/null", O_WRONLY);
        dup2(outputFD, STDOUT_FILENO);
        close(outputFD);
    }
}

void runOtherCommand (struct Command* cmd, int* lastStatus, int* lastSignal) {
    int childStatus;
    pid_t spawnPid = fork();

    if (spawnPid == -1) {
        perror("fork");
        exit(1);
    }

    if (spawnPid == 0) {
        struct sigaction childSIGINT = {0};
        struct sigaction childSIGTSTP = {0};

        if (cmd->background == 1) {
            childSIGINT.sa_handler = SIG_IGN;
        }
        else {
            childSIGINT.sa_handler = SIG_DFL;
        }

        sigfillset(&childSIGINT.sa_mask);
        childSIGINT.sa_flags = 0;
        sigaction(SIGINT, &childSIGINT, NULL); 

        childSIGTSTP.sa_handler = SIG_IGN;
        sigfillset(&childSIGTSTP.sa_mask);
        childSIGTSTP.sa_flags = 0;
        sigaction(SIGTSTP, &childSIGTSTP, NULL);

        setupInput(cmd);
        setupOutput(cmd);

        execvp(cmd->args[0], cmd->args);

        perror(cmd->args[0]);
        exit(1);
    }
    else {
        if (cmd->background == 1) {
            printf("background pid is %d\n", spawnPid);
            fflush(stdout);
            //addBackgroundPid(spawnPid);
        }
        else{
            waitpid(spawnPid, &childStatus, 0);

            if (WIFEXITED(childStatus)) {
                *lastStatus = WEXITSTATUS(childStatus);
                *lastSignal = 0;
            }
            else if (WIFSIGNALED(childStatus)) {
                *lastSignal = WTERMSIG(childStatus);
                printf("terminated by signal %d\n", *lastSignal);
                fflush(stdout);    
            }
        }
    }
}


/* runBuiltIn MAIN FUNCTIONALITIES ARE TO MEET THE ASSIGNMENT REQUIREMENTS
 * OF BEING ABLE TO RUN exit, cd, AND status IN THE TERMINAL. 
 * RETURNS 1 FOR BUILT-IN COMMAND AND 0 IF NOT */
int runBuiltIn (struct Command* cmd, int lastStatus, int lastSignal) {
    // Built-in commands use if statements since the inputs can conditionally be a built-in command
    // The if statements is efficient at checking if one of the built-in commands must run.

    // Checks for exit command and ends program if so
    if (strcmp(cmd->args[0], "exit") == 0) {
        //killBackgroundProcess();
        exit(0);
    }
    // Checks for cd command and changes directory accordingly if so
    if (strcmp(cmd->args[0], "cd") == 0) {
        if (cmd->args[1] == NULL) {
            chdir(getenv("HOME"));
        }
        else {
            chdir(cmd->args[1]);
        }
        return 1;
    }
    // Checks for status command and prints based on lastStatus
    // or lastSignal accordingly
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

        expandPID(input, expandedInput);
        //printf("%s\n", expandedInput);
        parseCommand(expandedInput, &cmd);

        if (cmd.args [0] == NULL) {
            continue;
        }

        if (runBuiltIn(&cmd, lastStatus, lastSignal) == 0) {
            runOtherCommand(&cmd, &lastStatus, &lastSignal);
        }
    }
    return 0;
}

