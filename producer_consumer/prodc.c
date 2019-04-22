#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <memory.h>
#include <unistd.h>
#include <error.h>
#include <pthread.h>
#include "prodcon.h"


static int producer_count;
static int consumer_count;
run_producer_f run_producer;
run_consumer_f run_consumer;
pthread_mutex_t lock[];
char **new_argv;
int new_argc = 0;
int counter = 0;


struct producers_arg {
    int id;
    char **new_argv;
    int new_argc;
} ProducersArgs;

struct consumers_arg {

} ConsumersArgs;


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


// static int my_consumer_number;

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
    // char *str = pop(&heads[my_consumer_number]);
    char *str = pop(&heads[counter]);
    return str;
}

void do_usage(char *prog)
{
    printf("USAGE: %s shared_lib consumer_count producer_count ....\n", prog);
    exit(1);
}

void * thread_Func(void * ptr) {

}

void * thread_producer(void * id){
    int i = *(int *) id;
    run_producer(i, producer_count, produce, new_argc, new_argv);
    counter++;
    pthread_mutex_unlock(&lock);
    
}

void * thread_consumer(void * id){
    int i = *(int *) id;
    run_consumer(i, consume, new_argc, new_argv);
    counter++;
    pthread_mutex_unlock(&lock);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        do_usage(argv[0]);
    }

    char *shared_lib = argv[1];
    consumer_count = strtol(argv[2], NULL, 10);
    producer_count = strtol(argv[3], NULL, 10);

    new_argv = &argv[4];
    new_argc = argc - 4;
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
    pthread_t consumers[consumer_count]; //initialize the conumsers
    pthread_mutex_init(&lock,NULL); //initialize the lock 

    heads = calloc(consumer_count, sizeof(*heads));

    while(1){
        pthread_mutex_lock(&lock[0]);
    }

    // while(1){
    //     pthread_mutex_lock(&lock);
    //     if(counter >= producer_count){
    //         // printf("Counter %d has reached producer_count of %d!\n",counter,producer_count);
    //         pthread_mutex_unlock(&lock);
    //         break;
    //     } else if(pthread_create(&producers[counter], NULL, thread_producer, &counter) != 0){
    //         perror("Thread creation");
    //         exit(EXIT_FAILURE);
    //     }

    // }

    // // puts("Just checking if this continues without waiting for the producers...");
    // counter = 0;
    // while(1){
    //     pthread_mutex_lock(&lock);
    //     if(counter >= consumer_count){
    //         // printf("Counter %d has reached consumer_count of %d!\n",counter,consumer_count);
    //         pthread_mutex_unlock(&lock);
    //         break;
    //     } else if(pthread_create(&consumers[counter], NULL, thread_consumer, &counter) != 0){
    //         perror("Thread creation");
    //         exit(EXIT_FAILURE);
    //     }

    // }



    //This will wait for the producers to finish running
    int c = 0;
    do {
        pthread_join(producers[c], NULL);
        c++;
    } while(c < consumer_count);
    //This will wait for the consumers to finish running
    c = 0;
    do {
        pthread_join(consumers[c], NULL);
        c++;
    } while(c < consumer_count);
    pthread_mutex_destroy(&lock);

    return 0;
}