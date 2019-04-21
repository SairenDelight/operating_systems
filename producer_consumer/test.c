#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include <error.h>

short signal[5] = {0};
int counter = 0;
int values[10] = {0};
pthread_mutex_t lock;
void * threadPrint(void * id){
    int * idNumPtr = id;
    int idNum = *idNumPtr;
    printf("Printing on the thread %d!\n",idNum);
    counter++;
    pthread_mutex_unlock(&lock);
}


int main(int argc, char* argv[]){
    if ( argc < 2){
        printf("USAGE: %s thread_count ....\n", argv[0]);
        exit(1);
    }
    pthread_mutex_init(&lock, NULL);
    int thread_count = strtol(argv[1], NULL, 10);
    pthread_t thread_id[thread_count];
    // pthread_mutex_lock(&lock);
    while(1){
        pthread_mutex_lock(&lock);
        if(counter >= thread_count){
            printf("Counter %d has been reached!\n",counter);
            break;
        } else if(pthread_create(&thread_id[counter], NULL, threadPrint, &counter) != 0){ //detect for thread creation failure
            perror("Thread creation");
            exit(EXIT_FAILURE);
        }
    }


    int c = 0;
    do {
        pthread_join(thread_id[c],NULL); //this will wait for the thread to finish
        c++;
    } while(c < thread_count);
    pthread_mutex_destroy(&lock);
    return 0;
}