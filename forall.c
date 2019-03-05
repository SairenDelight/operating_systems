#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

static pid_t child; // to get the child process id
static int sigNum = 0; // signal number
static int saved_stdout; // used to save the output fd to terminal output
int main(int argc, char *argv[]) {
    if(argc < 3){
        puts("Too little arguments");
        return -1;
    } else {
        int intlen(int); //declaring function to count length of digit
        void handler(int); // declaring function handle signal interrupts
        saved_stdout = dup(1);
        
        struct sigaction sa;
        sa.sa_handler = &handler; //give handler function to replace default
        sa.sa_flags = 0; //default
        int length = argc;
        for(int i = 2, c = 1; i < argc; i++, c++){
            int length = intlen(c); //Used to name the file
            int totallen = length + 5;
            char num[totallen];
            snprintf(num,sizeof(num),"%d",c);
            char ext[5] = ".out";
            strcat(num,ext);

            int fd = open(num, O_RDWR | O_CREAT | O_APPEND); //opens the file
            if(fd == -1){
                puts("Unable to open file. Most likely another file exists already with this name. Please remove those files.");
            }

            dup2(fd,1);
            if(sigaction(SIGINT, &sa,NULL) == -1){
                perror("sigaction SIGINT");
                exit(errno);
            }

            if(sigaction(SIGQUIT, &sa,NULL) == -1){
                perror("sigaction SIGQUIT");
                exit(errno);
            }

            child = fork();
            
            if(child == 0){
                // printf("Executing %s %s\n", argv[1],argv[i]); // a reminder how horrible this was to use for signals (not signal safe)
                // fflush(stdout);
                dprintf(fd,"Executing %s %s\n", argv[1],argv[i]);
                execlp(argv[1],argv[1],argv[i],NULL);
            } else {
                int stat;
                wait(&stat);
                if(WIFEXITED(stat) == 0 || sigNum == 0){
                    dprintf(fd,"Finished executing %s %s exit code = %d\n", argv[1],argv[i],WEXITSTATUS(stat));
                }else {
                    dprintf(fd,"Stopped executing %s %s signal = %d\n", argv[1],argv[i],sigNum);
                }
                sigNum = 0;
                close(fd);
            }

        }
        return 0;
    }
}

int intlen(int input) {
    int count = 1;
    while(input >= 10) {
        input/=10;
        count++;
    }
    return input;
}

void handler(int num) {
    switch(num) {
        case SIGINT:
            // printf("Signaling %d\n", child);
            dprintf(saved_stdout,"Signaling %d\n", child); //Jeffrey Wu recommended the dprints instead of printf and fflush
            break;
        case SIGQUIT:
            dprintf(saved_stdout,"Signaling %d\n",getpid()); 
            // printf("Signaling %d\n",getpid());
            // fflush(stdout);
            dprintf(saved_stdout,"Exiting due to quit signal\n");
            exit(0);
            break;
        default:
        break;
    }
    sigNum = num;
}