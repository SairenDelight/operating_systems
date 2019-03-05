#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

static pid_t child;
int sigNum = 0;
int main(int argc, char *argv[]) {
    if(argc < 3){
        puts("Too little arguments");
        return -1;
    } else {
        int intlen(int);
        void handler(int);
        int saved_stdout;
        saved_stdout = dup(1);
        
        struct sigaction sa;
        sa.sa_handler = &handler;
        sa.sa_flags = 0;
        int length = argc;
        for(int i = 2, c = 1; i < argc; i++, c++){
            int length = intlen(c);
            int totallen = length + 5;
            char num[totallen];
            snprintf(num,sizeof(num),"%d",c);
            char ext[5] = ".out";
            strcat(num,ext);

            int fd = open(num, O_RDWR | O_CREAT | O_APPEND);
            if(fd == -1){
                puts("Unable to open file.");
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
                setpgid(getpid(),getpid());
                printf("This is the child process %d\n", getpid());
                fflush(stdout);
                printf("Executing %s %s\n", argv[1],argv[i]);
                fflush(stdout);
                execlp(argv[1],argv[1],argv[i],NULL);
            } else {
                // close(fd);
                // dup2(saved_stdout,1);
                int stat;
                wait(&stat);
                // close(saved_stdout);

                // dup2(fd,1);
                if(WIFEXITED(stat) == 0 || sigNum == 0){
                    // printf("Finished executing %s %s exit code = %d\n", argv[1],argv[i],WEXITSTATUS(stat));

                    // fflush(stdout);
                    dprintf(fd,"Finished executing %s %s exit code = %d\n", argv[1],argv[i],WEXITSTATUS(stat));
                }else {
                    // printf("Stopped executing %s %s signal = %d\n", argv[1],argv[i],sigNum);
                    // fflush(stdout);
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
            printf("Signaling %d\n", child);
            break;
        case SIGQUIT:
            printf("Signaling %d\n",getpid());
            fflush(stdout);
            printf("Exiting due to quit signal\n");
            fflush(stdout);
            exit(0);
            break;
        default:
        break;
    }
    sigNum = num;
}