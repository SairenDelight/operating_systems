#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include <error.h>

short signal[5] = {0};
int counter = 0;
int con_counter  = 0;
int values[10] = {0};
pthread_mutex_t lock[2];
pthread_cond_t cond[5] = {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }; 


void produce() {
    printf("UGH LABOR PRODUCING!\n");
}

void consume(){
    printf("NOM NOM NOM!\n");
}

void * threadFunc(void * id){
    pthread_mutex_lock(&lock[0]);
    int * idNumPtr = id;
    int idNum = *idNumPtr;
    printf("The signal value is signal[%d] = %d\n",idNum, signal[idNum]);
    if(signal[idNum] == 1){
        counter++;
        printf("Printing on the thread %d!\n",idNum);
        produce();
        printf("Signaling to consumer on the thread %d!\n",idNum);
        pthread_cond_signal(&cond[idNum]); 
    } else {
        con_counter++;
        printf("Printing on the thread con %d so I am waiting no dur!\n",idNum);
        signal[idNum]++;
        pthread_cond_wait(&cond[idNum], &lock[0]); 
        printf("Finishing up the code lalalala of consumer %d\n", idNum);
        consume();
    }
    pthread_mutex_unlock(&lock[0]);
    pthread_exit(NULL);
}

static int consumer_count;
static int producer_count;

int main(int argc, char* argv[]){
    if ( argc < 2){
        printf("USAGE: %s thread_count ....\n", argv[0]);
        exit(1);
    }

    // consumer_count = strtol(argv[2], NULL, 10);
    // producer_count = strtol(argv[3], NULL, 10);

    pthread_mutex_init(&lock[0], NULL);
    pthread_mutex_init(&lock[1], NULL);
    int thread_count = strtol(argv[1], NULL, 10);
    pthread_t thread_id[thread_count];
    pthread_t thread_con[thread_count];



    int i = 0;

    while(1){
        // pthread_mutex_lock(&lock[0]);
        if(i >= thread_count){
            // pthread_mutex_unlock(&lock[0]);
            // counter = 0;
            break;
        } else if(pthread_create(&thread_id[i], NULL, threadFunc, &counter) != 0){ //detect for thread creation failure
            perror("Thread creation");
            exit(EXIT_FAILURE);
        } 
        i++;
    }


    // puts("WHEEE");

    i = 0;
    while(1){
        // pthread_mutex_lock(&lock[1]);
        if(i >= thread_count){
            // pthread_mutex_unlock(&lock[1]);
            break;
        } else if(pthread_create(&thread_con[i], NULL, threadFunc, &con_counter) != 0){ //detect for thread creation failure
            perror("Thread creation con");
            exit(EXIT_FAILURE);
        }
        i++;
    }



    int c = 0;
    do {
        pthread_join(thread_id[c],NULL); //this will wait for the thread to finish
        printf("Do my producer children return: %d\n\n",c);
        c++;
    } while(c < thread_count);
    c = 0;
      do {
        pthread_join(thread_con[c],NULL); //this will wait for the thread to finish
        printf("Do my consumer children return: %d\n\n",c);
        c++;
    } while(c < thread_count);
    printf("Does this print after running the thing before?");
    pthread_mutex_destroy(&lock[0]);
    pthread_mutex_destroy(&lock[1]);
    return 0;
}