/* $begin shellmain */
#include "myshell.h"
#include <stdio.h>

#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
char* remove_quote(char * str);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
        printf("CSE4100-SP-P2> ");                   
        fgets(cmdline, MAXLINE, stdin); 
        if (feof(stdin))
            exit(0);

        /* Evaluate */
        eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char* flag = strchr(cmdline, '|');
    if (flag == NULL) {
        char *argv[MAXARGS]; /* Argument list execve() */
        char buf[MAXLINE];   /* Holds modified command line */
        int bg;              /* Should the job run in bg or fg? */
        pid_t pid;           /* Process id */
        
        strcpy(buf, cmdline);
        bg = parseline(buf, argv); 
        if (argv[0] == NULL)  
            return;   /* Ignore empty lines */
        if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
            pid = Fork();
            if (pid == 0) {
                if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
                    printf("%s: Command not found.\n", argv[0]);
                    exit(0);
                }
            }

            if (!bg){ 
                int status;
                Waitpid(pid, &status,0);
            }
            else//when there is backgrount process!
                printf("%d %s", pid, cmdline);
        }
	/* Parent waits for foreground job to terminate */
    }
    else {
        char* command[MAXARGS];
        char* cmd;
        char buf[MAXLINE];
        strcpy(buf, cmdline);
        int len = strlen(cmdline) - 1;
        int num_pipe = 0;
        for (int i = 0; i < len; i++){
            if (cmdline[i] == '|')
                num_pipe++;
        }
        cmd = strtok(buf, "|");
        for (int i = 0; i < num_pipe + 1; i++) {
            command[i] = cmd;
            cmd = strtok(NULL, "|");
        }
        int pipes[num_pipe][2];
        for (int i = 0; i < num_pipe; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                return;
            }
        }
        char* argv[MAXARGS];
        int bg; 
        pid_t pid;
        int prev_pipe[2]; 
        for (int i = 0; i < num_pipe + 1; i++) {
            if ((pid = Fork()) == 0) {
                if (i != 0) {
                    Dup2(prev_pipe[0], STDIN_FILENO);
                    close(prev_pipe[1]);
                }
                if (i != num_pipe) {
                    Dup2(pipes[i][1], STDOUT_FILENO);
                    close(pipes[i][0]);
                }
                for (int j = 0; j < num_pipe; j++) {
                    close(pipes[j][0]); 
                    close(pipes[j][1]);
                }
                parseline(command[i], argv);
                if (!builtin_command(argv)) {
                    if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
                        printf("%s: Command not found.\n", argv[0]);
                        exit(0);
                    }
                }
                else {
                    exit(0);
                }
            }
            else {
                if (i != 0) {
                    close(prev_pipe[0]);
                    close(prev_pipe[1]);
                }
                if (i != num_pipe) {
                    prev_pipe[0] = pipes[i][0];
                    prev_pipe[1] = pipes[i][1];
                }

                if (!bg){ 
                    int status;
                    Waitpid(pid, &status,0);
                }
                else//when there is backgrount process!
                    printf("%d %s", pid, cmdline);
            }
        }
        for (int i = 0; i < num_pipe; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "cd")) {
        if (argv[1] == NULL) 
            chdir(getenv("HOME"));
        else  {
            if (!strcmp(argv[1],"~")) {
                chdir(getenv("HOME"));
                return 1;
            }
            if (argv[2] != NULL) {
                printf("bash: cd: too many arguments\n");
                return 1;
            }
            if (chdir(argv[1]) != 0) {
                printf("bash: cd: %s: No such file or directory\n", argv[1]);
            }
        }
        return 1;
    }
    if (!strcmp(argv[0], "quit")) /* quit command */
	    exit(0); 
    if (!strcmp(argv[0], "exit"))
        exit(0);
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
                buf++;
    }
    argv[argc] = NULL;
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '"') 
            argv[i] = remove_quote(argv[i]);
    }
    if (argc == 0)  /* Ignore blank line */
	    return 1;
    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	    argv[--argc] = NULL;

    return bg;
}
/* $end parseline */

char* remove_quote(char* str) {
    int len = strlen(str);
    if (len <= 0) return str;
    int start = 0; int end = len - 1;
    return strndup(str+start+1, end-start-1);    
}

pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
int Dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 error");
    return rc;
}
pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}