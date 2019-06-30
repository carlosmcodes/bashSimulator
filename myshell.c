/****************************************************************
 * Name        :  Carlos Lopez                                  *
 * Class       :  CSC 415                                       *
 * Date        :  [Started] 3/10/19: [Finished] 3/19/19         *
 * Description :  Writing a simple bash shell program           *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <errno.h>

/* CANNOT BE CHANGED */
#define BUFFERSIZE 256
/* --------------------*/
#define PROMPT "myShell >> "
#define PROMPTSIZE sizeof(PROMPT)

void parser(char *str, char **argVec);

void clear_buffer(char *str, char **argVec);

void run_cd(char **argVec);

void redirect_operator(int i, char **argVec);

void redirect_Input_operator(int i, char **argVec);

void append_operator(int i, char **argVec);

int initRedirect(char **argVec);

int split_pipes(char **argVec);

void run_Execvp(char **argVec, int argCount);

void ampersand(char **argVec);

void run_prompt();

// ----------------- Globals ------------------
char **argVec;
int argCount;
char input[BUFFERSIZE];

int main(int *argc, char **argv) {
    argVec = malloc(10 * sizeof(char *));
    int flipamp = 0;
    run_prompt();
    while (strcmp(fgets(input, sizeof(input), stdin), "exit\n") != 0) {
        parser(input, argVec);
        if (strcmp(argVec[argCount], "&") == 0) {
            ampersand(argVec);
            flipamp = -1;
        }
        if (strcmp(input, "pwd\n") == 0) {
            printf("%s\n", getcwd(input, sizeof(input)));
        } else if (strcmp(argVec[0], "cd") == 0) {
            run_cd(&argVec[argCount]);
        } else if (initRedirect(argVec) == 0) {
            if (split_pipes(argVec) == 0)
                if (flipamp != -1)
                    run_Execvp(argVec, argCount);
        }
        flipamp = 0;
        clear_buffer(input, argVec);
        run_prompt();
//        printf(PROMPT);
    };

    return 0;
}

void parser(char *str, char **argVec) {

    char *tokens;
    int index_Of_argv = 0;
    char *newtoken = malloc(10 * sizeof(char));

    while ((tokens = strtok_r(str, " ", &str))) {
        if (tokens[strlen(tokens) - 1] == '\n') {
            strncpy(newtoken, tokens, (int) (strlen(tokens) - 1));
            argVec[index_Of_argv] = newtoken;
        } else {
            argVec[index_Of_argv] = tokens;
        }
        index_Of_argv++;
    }
    argVec[index_Of_argv] = NULL;
    index_Of_argv--;
    argCount = index_Of_argv;
}

void clear_buffer(char *str, char **argVec) {
    memset(str, 0, sizeof(str));
    *argVec = 0;
    argCount = 0;
}

void run_Execvp(char **argVec, int argCount) {

    int fd[2];

    //{pipe} communication between 2 processes
    pipe(fd);
    pid_t process;

    // {fork} creation of 2 processes
    process = fork();

    if (process < 0)
        perror("fork failed\n");
    if (process == 0) {
        close(0);
        dup(fd[0]);
//        puts("child");
        execvp(argVec[0], argVec);
        close(0);
        close(1);
        perror("");
        return;
    }
    wait(0);
}

void run_cd(char **argVec) {
    char *previousDir;
    if (argCount == 0) {
        chdir(getenv("HOME"));
//        printf("%s\n", getcwd(input, sizeof(input)));
//        printf("HOME : %s\n", getenv("HOME"));
    } else {
        if (strcmp(argVec[0], "..") == 0) {
            chdir(argVec[0]);
//            printf("%s\n", getcwd(input, sizeof(input)));
        } else {
            previousDir = getcwd(input, sizeof(input));
            strcat(previousDir, "/");
            strcat(previousDir, argVec[0]);
            chdir(previousDir);
//            printf("%s\n", getcwd(input, sizeof(input)));
        }
    }
}

int initRedirect(char **argVec) {
    for (int i = 0; i < argCount; i++) {
        if (strcmp(argVec[i], ">") == 0) {
            argVec[i] = NULL;
            redirect_operator(i, argVec);
            return i;
        }

        if (strcmp(argVec[i], "<") == 0) {
            argVec[i] = NULL;
            redirect_Input_operator(i, argVec);
            return i;
        }
        if (strcmp(argVec[i], ">>") == 0) {
            argVec[i] = NULL;
            append_operator(i, argVec);
            return i;
        }
    }
    return 0;
}

void redirect_operator(int i, char **argVec) {
    int fd[2];
    pipe(fd);
    pid_t process;
    int readfd = open(argVec[i + 1], O_RDWR | O_CREAT | O_EXCL | O_TRUNC, S_IRWXU);
    perror("");
    process = fork();

    if (process < 0)
        perror("fork failed\n");
    if (process == 0) {
        if (readfd > 0) {
            dup2(readfd, 1);
            close(readfd);
            execvp(argVec[0], argVec);
        }
    } else {
        wait(0);
    }
}


void redirect_Input_operator(int i, char **argVec) {
    int fd[2];
    pipe(fd);
    pid_t process;
    int readfd = open(argVec[i + 1], O_RDONLY | O_CREAT, S_IRWXU);
//    perror("no work\n");
    process = fork();

    if (process < 0)
        perror("fork failed\n");
    if (process == 0) {
        if (readfd > 0) {
            dup2(readfd, STDIN_FILENO);
            close(readfd);
            execvp(argVec[0], argVec);
//            perror("redirect backwRDS\n");
        }
    } else {
        wait(0);
    }
}

void append_operator(int i, char **argVec) {
    int fd[2];
    pipe(fd);
    pid_t process;
    int readfd = open(argVec[i + 1], O_RDWR | O_APPEND);
    process = fork();

    if (process < 0)
        perror("fork failed\n");
    if (process == 0) {
        if (readfd > 0) {
            dup2(readfd, 1);
            close(readfd);
            execvp(argVec[0], argVec);
        }
    } else {
        wait(0);
    }
}

int split_pipes(char **argVec) {

    char **aVec1 = malloc(10 * sizeof(char *));
    int aCount1 = 0;
    for (int i = 0; i < 4; i++) {
        aVec1[i] = malloc(10 * sizeof(char *));
    }

    char **aVec2 = malloc(10 * sizeof(char *));
    int aCount2 = 0;
    for (int i = 0; i < 4; i++) {
        aVec2[i] = malloc(10 * sizeof(char *));
    }
    int j = 0;

    // ------------- ONE ---------
    for (int i = 0; i < argCount; i++) {
        if (strcmp(argVec[i], "|") == 0) {
            j = i;
            break;
//            return i;
        }
        aVec1[i] = argVec[i];
        aCount1++;
//        puts(aVec1[i]);
        if (i == argCount - 1)
            return 0;
    }
    aVec1[aCount1] = NULL;
    int k = 0;
    //------------- TWO ----------
    for (int i = j + 1; i <= argCount; i++) {
        aVec2[k] = argVec[i];
        aCount2++;
        k++;
    }
    aVec2[aCount2] = NULL;

    int fd[2];
    pid_t pid;
    pipe(fd);
    //child
    pid = fork();
    if (pid < 0)
        perror("fork1 failed\n");
    if (pid == 0) {
        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        execvp(aVec1[0], aVec1);
        perror("avec1 no work:\n");
    }
    wait(0);
    pid = fork();

    if (pid < 0)
        perror("fork2 failed\n");
    if (pid == 0) {
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
//        char* right_side[] = {"wc","-l",NULL};
        execvp(aVec2[0], aVec2);
        perror("avec2 no work:\n");
    }

    close(fd[0]);
    close(fd[1]);
    wait(0);
    return j;
}

void ampersand(char **argVec) {
    argVec[argCount] = NULL;
    pid_t pid;
    int fd[2];
    pid = fork();
    if (pid < 0)
        perror("fork failed\n");
    if (pid == 0) {
        dup2(0, 1);
        close(0);
//        close(1);
//        puts("child");
        execvp(argVec[0], argVec);
        perror("");
    } else {

    }

    printf("[1] %d\n", pid);
    wait(0);

//    close(0);

}

void run_prompt() {
    char *prompt_plus;
    char *prompt_one = malloc(100* sizeof(char));
    strncpy(prompt_one, PROMPT, PROMPTSIZE - 4);
    prompt_plus = getcwd(input, sizeof(input));
    strncat(prompt_one, prompt_plus, strlen(prompt_plus));
    strcat(prompt_one, " >> ");
    printf("%s", prompt_one);

}