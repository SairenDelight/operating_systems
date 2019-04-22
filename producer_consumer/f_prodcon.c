#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <memory.h>
#include <unistd.h>
#include <error.h>
#include <pthread.h>
#include "prodcon.h"

typedef struct producers_arg {
    int id;
    char **new_argv;
    int new_argc;
} prodArg;

static int producer_count;
static int consumer_count;
run_producer_f run_producer;
run_consumer_f run_consumer;
prodArg * producer_args;
prodArg * consumer_args;
pthread_mutex_t * lock;
pthread_mutex_t prodLock;
pthread_cond_t * cond;
int * isProducer;



struct llist_node {
    struct llist_node *next;
    char *str;
};




/**
 * pop a node off the start of the list.
 *
 * @param phead the head of the list. this will be modified by the call unless the list is empty
 * (*phead == NULL).
 * @return NULL if list is empty or a pointer to the string at the top of the list. the caller is
 * incharge of calling free() on the pointer when finished with the string.
 */
char *pop(struct llist_node **phead)
{
    if (*phead == NULL) {
        return NULL;
    }
    char *s = (*phead)->str;
    struct llist_node *next = (*phead)->next;
    free(*phead);
    *phead = next;
    return s;
}

/**
 * push a node onto the start of the list. a copy of the string will be made.
 * @param phead the head of the list. this will be modified by this call to point to the new node
 * being added for the string.
 * @param s the string to add. a copy of the string will be made and placed at the beginning of
 * the list.
 */
void push(struct llist_node **phead, const char *s)
{
    struct llist_node *new_node = malloc(sizeof(*new_node));
    new_node->next = *phead;
    new_node->str = strdup(s);
    *phead = new_node;
}

// the array of list heads. the size should be equal to the number of consumers
static struct llist_node **heads;

static assign_consumer_f assign_consumer;


static int my_consumer_number;

void queue(int consumer, const char *str)
{
    push(&heads[consumer], str);
}

void produce(const char *buffer)
{
    int hash = assign_consumer(consumer_count, buffer);
    queue(hash, buffer);
}

char *consume() {
    char *str = pop(&heads[my_consumer_number]);
    // char *str = pop(&heads[consumer_args->id]);
    // printf("CONSUMER ID IN CONSUME FUNCTION IS: %d with data %s\n",my_consumer_number,str);
    return str;
}

void do_usage(char *prog)
{
    printf("USAGE: %s shared_lib consumer_count producer_count ....\n", prog);
    exit(1);
}

void * thread_Produce(void * ptr){
    prodArg * temp = ptr;
    int idNum = temp->id;
     if(pthread_mutex_lock(&prodLock) != 0){
        perror("Mutex Lock in thread_Func");
        exit(EXIT_FAILURE);
    }
    temp->id++;
    run_producer(idNum, producer_count, produce, temp->new_argc, temp->new_argv);

    if(pthread_mutex_unlock(&prodLock) != 0){
        perror("Mutex Unlock in thread_Func");
        exit(EXIT_FAILURE);
    }
    pthread_exit(NULL);
}

void * thread_Consume(void * ptr){
     prodArg * temp = ptr;
    int idNum = temp->id;
     if(pthread_mutex_lock(&lock[idNum]) != 0){
        perror("Mutex Lock in thread_Func");
        exit(EXIT_FAILURE);
    }
    temp->id++;
    isProducer[idNum]++;
    pthread_cond_wait(&cond[idNum], &lock[idNum]); 
    my_consumer_number = idNum;
    run_consumer(idNum, consume, temp->new_argc, temp->new_argv);


    if(pthread_mutex_unlock(&lock[idNum]) != 0){
        perror("Mutex Unlock in thread_Func");
        exit(EXIT_FAILURE);
    }
    pthread_exit(NULL);
}

void * thread_Func(void * ptr) {
    prodArg * temp = ptr;
    int idNum = temp->id;
    if(pthread_mutex_lock(&lock[idNum]) != 0){
        perror("Mutex Lock in thread_Func");
        exit(EXIT_FAILURE);
    }
    if(isProducer[idNum] == 1){
        temp->id++;
        run_producer(idNum, producer_count, produce, temp->new_argc, temp->new_argv);
    } else {
        temp->id++;
        isProducer[idNum]++;
        pthread_cond_wait(&cond[idNum], &lock[idNum]); 
        my_consumer_number = idNum;
        run_consumer(idNum, consume, temp->new_argc, temp->new_argv);
    }

    if(pthread_mutex_unlock(&lock[idNum]) != 0){
        perror("Mutex Unlock in thread_Func");
        exit(EXIT_FAILURE);
    }
    pthread_exit(NULL);
}


int main(int argc, char **argv)
{
    if (argc < 4) {
        do_usage(argv[0]);
    }

    char *shared_lib = argv[1];
    consumer_count = strtol(argv[2], NULL, 10);
    producer_count = strtol(argv[3], NULL, 10);

    char **new_argv = &argv[4];
    int new_argc = argc - 4;
    setlinebuf(stdout);

    if (consumer_count <= 0 || producer_count <= 0) {
        do_usage(argv[0]);
    }

    void *dh = dlopen(shared_lib, RTLD_LAZY);

    // load the producer, consumer, and assignment functions from the library
    run_producer = dlsym(dh, "run_producer");
    run_consumer = dlsym(dh, "run_consumer");
    assign_consumer = dlsym(dh, "assign_consumer");
    if (run_producer == NULL || run_consumer == NULL || assign_consumer == NULL) {
        printf("Error loading functions: prod %p cons %p assign %p\n", run_producer,
                run_consumer, assign_consumer);
        exit(2);
    }

    pthread_t producers[producer_count]; //initialize the producers
    pthread_t consumers[consumer_count]; //initialize the consumers
    isProducer = malloc(sizeof(int) * consumer_count);
    lock = malloc(sizeof(pthread_mutex_t) * consumer_count);
    cond = malloc(sizeof(pthread_cond_t) * consumer_count);
    pthread_mutex_init(&prodLock, NULL);
    // puts("MADE IT HERE");
    for(int i = 0 ; i < consumer_count; i++){
        pthread_mutex_init(&lock[i],NULL);
        pthread_cond_init(&cond[i], NULL);
    }



    heads = calloc(consumer_count, sizeof(*heads));
    producer_args = malloc(sizeof(*producer_args));
    producer_args->id = 0;
    producer_args->new_argv = new_argv;
    producer_args->new_argc = new_argc;
    
    consumer_args = malloc(sizeof(*consumer_args));
    consumer_args->id = 0;
    consumer_args->new_argv = new_argv;
    consumer_args->new_argc = new_argc;


    int i = 0;
    while(1){
        if(i >= producer_count){
            producer_args->id = 0;
            break;
        } else {
            // int pthread = pthread_create(&producers[i], NULL,thread_Func, producer_args);
            int pthread = pthread_create(&producers[i], NULL,thread_Produce, producer_args);
            if(pthread != 0){
                perror("Thread creation");
                exit(EXIT_FAILURE);
            }
        }
        i++;
    }

    i = 0;
    while(1){
        if(i >= consumer_count){
            break;
        } else {
            int pthread = pthread_create(&consumers[i], NULL,thread_Consume, consumer_args);
            // int pthread = pthread_create(&consumers[i], NULL,thread_Func, consumer_args);
            if(pthread != 0){
                perror("Thread creation");
                exit(EXIT_FAILURE);
            }

        }
        i++;
    }


    //This will wait for the producers to finish running
    int c = 0;
    do {
        // printf("Before it starts join of %d\n",c);
        pthread_join(producers[c], NULL);
        // printf("Do my producer children return: %d\n",c);
        c++;
    } while(c < producer_count);

    // sleep(2);
    for(int count = 0 ; count < consumer_count; count++){
        // printf("The CONDITION SIGNAL SENDING is %d\n",count);
        pthread_cond_signal(&cond[count]); 
    }


    // This will wait for the consumers to finish running
    c = 0;
    do {
        pthread_join(consumers[c], NULL);
        // printf("Do my consumer children return: %d\n",c);
        c++;
    } while(c < consumer_count);
   
    for(int x = 0 ; x < consumer_count; x++){
        pthread_mutex_destroy(&lock[x]);
    }
    pthread_mutex_destroy(&prodLock);

    free(producer_args);
    free(lock);
    free(isProducer);
    free(cond);
    free(consumer_args);
    return 0;
}