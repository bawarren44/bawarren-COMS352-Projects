#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE 80 /* The maximum length command*/

/**
 * Struct that contains all information needed about a process
 */
struct process{
    int pid;
    char *argu;
    int entranceNumber;
    struct process *next;
};

struct process *head = NULL;
struct process *current = NULL;

/**
 * Inserts process to front of list
 * @param pid
 * @param argu
 * @param entranceNumber
 */
void insert(int pid, char *argu, int entranceNumber) {
    struct process *inserty = (struct process*) malloc(sizeof(struct process));
    inserty->pid = pid;
    inserty->argu = argu;
    inserty->entranceNumber = entranceNumber;
    if(head == NULL){
        head = inserty;
    }else{
        current->next = inserty;
    }
}

/**
 * Delete a process from the process list
 * @param pid
 * @return
 */
struct process* delete(int pid) {
    struct process* cur = head;
    struct process* previous = NULL;
    if(head == NULL) { /* if head is empty */
        return NULL;
    }
    while(cur->pid != pid) { /* traversing through the list */
        if(cur->next == NULL) {
            return NULL;
        } else {
            previous = cur;
            cur = cur->next;
        }
    }
    if(cur == head) {
        head = head->next;
    } else {
        previous->next = cur->next;
    }
    return cur;
}

/**
 * For clearing the argument array each time the while loop wants new user intake
 * @param args - array of arguments
 * @param buffer - copy of file line
 * @return
 */
void clearArray(char *args[],char buffer[]){
    /* clear argument array */
    int i = 0;
    while(i < MAX_LINE){
        if(i < MAX_LINE/2 +1){
            args[i] = "\0";
        }
        buffer[i] = '\0';
        i++;
    }
}

/**
 * For setting up the arg array to use in execvp() function
 * @param args - array of arugments
 * @param buffer - copy of file line
 * @return
 */
void setCommandArg(char *args[], char buffer[], int *redirect, char *inOrOut){
    int count = 0; /* count of index of args */
    args[count++] = strtok(buffer," "); /* splits buffer into arguments */
    while(args[count-1] != NULL){ /* get file input into args array */
        char *ray = strtok(NULL, " ");
        int len = 0;
        if(ray != NULL) {
            len = strlen(ray);
            if (ray[len - 1] == '\n') {
                ray[len - 1] = '\0';
            }
        }
        if(ray != NULL && strrchr(ray,'&') != NULL){ /* accounting for if its a background process */
            ray = strtok(NULL, " ");
            args[count++] = NULL;
        }
        if(ray != NULL && strrchr(ray,'>') != NULL){ /* in case of redirect case */
            *redirect = 2;
            ray = strtok(NULL, " ");
            strcpy(inOrOut,ray);
            args[count++] = NULL;
            break;
        }else if(ray != NULL && strrchr(ray, '<') != NULL) { /* in case of redirect case */
            *redirect = 1;
            ray = strtok(NULL, " ");
            strcpy(inOrOut,ray);
            args[count++] = NULL;
            break;
        }else{
            args[count++] =  ray;/* gets next argument of buffer */
        }
    }
    if(args[0][0] != '\0' && args[1]== NULL){ /* remove end of line character at end of string */
        int len = strlen(args[0]);
        if(args[0][len -1] == '\n'){
            args[0][len -1] = '\0';
        }
    }
    int len = strlen(inOrOut);
    if(inOrOut[len -1] == '\n'){ /* remove end of line character at end of string */
        inOrOut[len -1] = '\0';
    }
}

/**
 * Main method containing the while loop that keeps my shell going. Loop continues until
 * User types "exit".
 * @return
 */
int main(void) {
    char *args[MAX_LINE/2 +1];  /* command line arguments */
    char buffer[MAX_LINE]; /* used to get file line */
    int should_run = 1; /* flag to determine when to exit program */
    int background = 0; /* If 1 then we have a background process */
    int redirect = 0; /* If 1 redirects input if 2 redirects output */
    int backIdx = 0; /* length of list of background processes */
    int backTotal = 1; /* entrance number associated with a background process */
    int status; /* status of process */

    while(should_run){ /* while loop for iterating the shell */
        /* shell start */
        printf("352>");
        fflush(stdout);
        background = 0;
        redirect = 0;
        clearArray(args,buffer);

        /**
        * After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) parent will invoke wait() unless command
        * included &
        */

        /* get command from input source */
        fgets(buffer, MAX_LINE, stdin);

        if(strcmp(buffer,"exit\n") == 0 || strcmp(buffer,"exit") == 0){
            break;
        }

        int bufflen = strlen(buffer);
        if(strrchr(buffer, '&') != NULL && buffer[bufflen -2] == '&'){ /* determines if process is a background process */
            background = 1;
        }

        char inOrOut[80]; /* used to determine redirect file name */
        memset(inOrOut,0,80);
        /* Get args to have command line arguments */
        setCommandArg(args, buffer, &redirect, inOrOut);

        /* kill command */
        if(strcmp(args[0],"kill") == 0) {  /* case of a kill command */
            int con = 0;
            current = head;
            while(current){
                int childpid = atoi(args[1]);
                if(childpid == current->pid) { /* kills the process with pid matching user input */
                    kill(childpid, SIGKILL);
                    printf("[%d] Terminated %s\n", current->entranceNumber, current->argu);
                    delete(childpid);
                    con = 1;
                    break;
                }
                current = current->next;
            }
            if(con == 1){ /* goes back to beginning of the while loop */
                continue;
            }
        }

        /* (1) fork a child process using fork() */
        pid_t pid;
        pid = fork(); /* creation of child process */

        if (pid < 0) { /* error occurred */
            printf("Fork Failed\n");
            return 1;
        } else if (pid == 0) { /* child process */
            char fileToUse[80];
            memset(fileToUse,0,80);
            strcpy(fileToUse,"./");
            strcat(fileToUse,inOrOut); /* sets fileToUse as redirect file */
            /* handles redirecting input or output according to redirect value */
            if(redirect == 1){
                int fd = open(fileToUse, STDIN_FILENO);
                dup2(fd, 0);
            }else if(redirect == 2){
                int fd = open(fileToUse, STDOUT_FILENO | O_CREAT, 0777);
                dup2(fd, 1);
            }
            /* (2) the child process will invoke execvp() */
            execvp(args[0], args);
            printf("%c",errno);
        } else{ /* parent process */
            /* (3) parent will invoke wait() unless command included & */
            if(background == 1) {
                printf("[%d] %d\n", backTotal, pid); /* printing background process */

                char *arr = malloc(sizeof(char)*80);
                int i = 0;
                while(args[i] != NULL) {
                    strcat(arr, args[i]);
                    strcat(arr, " ");
                    i++;
                }
                if(redirect == 1){
                    strcat(arr,"<");
                    strcat(arr, " ");
                    strcat(arr, inOrOut);
                }else if(redirect == 2){
                    strcat(arr,">");
                    strcat(arr, " ");
                    strcat(arr, inOrOut);
                }
                current = head;
                if(current != NULL && current->next){
                    while(current->next){
                        current = current->next;
                    }
                }
                insert(pid,arr,backTotal); /* adding new process */
                backTotal++;
            }else {
                waitpid(pid,&status,0);
            }
            current = head;
            while(current){ /* handling finished processes */
                int back = 0;
                int stat = 0;
                int result  = waitpid(current->pid, &stat, WNOHANG);
                if(result == current->pid){
                    if(stat == 0){
                        /* child finished */
                        printf("[%d] Done %s\n",current->entranceNumber,current->argu);
                        delete(current->pid);
                        back = 1;
                    }else{
                        /* child ran into an error */
                        printf("[%d] Exit %d %s\n",current->entranceNumber,WEXITSTATUS(stat),current->argu);
                        delete(current->pid);
                        back = 1;
                    }
                }
                if(back == 1){
                    current = head;
                }else{
                    current = current->next;
                }
            }
        }
    }
}
