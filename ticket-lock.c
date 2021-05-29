#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdatomic.h>

#define buff_size 15000000
#define TICKET_LOCK_INITIALIZER { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }

typedef struct ticket_lock {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    atomic_ullong queue_head, queue_tail;
} ticket_lock_t;

ticket_lock_t lock = TICKET_LOCK_INITIALIZER;

void ticket_lock(ticket_lock_t *ticket)
{
    unsigned long queue_me;

    pthread_mutex_lock(&(ticket->mutex));
    queue_me = ticket->queue_tail++;
    while (queue_me != ticket->queue_head)
    {
        pthread_cond_wait(&(ticket->cond), &(ticket->mutex));
    }

    pthread_mutex_unlock(&(ticket->mutex));
}

void ticket_unlock(ticket_lock_t *ticket)
{
    pthread_mutex_lock(&(ticket->mutex));
    ticket->queue_head++;
    pthread_cond_broadcast(&(ticket->cond));
    pthread_mutex_unlock(&(ticket->mutex));
}

typedef struct 
{
    char buffer[ buff_size ];
    unsigned writers;
    unsigned readers;
} library;

library Library = {{0}, 0, 0};

void print_library_state(unsigned id)
{
    //printf("\033[H\033[J");
    printf("[in(%u) r: %u w: %u \n", id, Library.readers, Library.writers);
}

void* readers_init(void* tid)
{
    unsigned id = *((unsigned*) tid)+1;

    while(true)
    {
        ticket_lock(&lock);

        ++Library.readers;
        print_library_state(id);

        for(size_t i = 0; i < buff_size; i++)
        {
            char c = Library.buffer[ i ];
        }

        --Library.readers;
        print_library_state(id);

        ticket_unlock(&lock);
    }
}

void* writers_init(void* tid)
{
    unsigned id = *((unsigned*) tid)+1;
    srandom(time(NULL));

    while(true)
    {
        ticket_lock(&lock);
        
        ++Library.writers;
        print_library_state(id);

        size_t pos = random() % (buff_size/2); // update at least half of the buffer
        for(size_t i = pos; i < buff_size; i++)
            Library.buffer[ i ] = (char) (random() % 255);

        --Library.writers;
        print_library_state(id);
        
        ticket_unlock(&lock);
    }
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        printf("Usage: ./rwp [Number_of_Readers] [Number_of_Writers]\n");
        return 0;
    }

    size_t NoReaders = atoi(argv[1]);
    size_t NoWriters = atoi(argv[2]);

    pthread_t* ReaderThreads = calloc(sizeof(pthread_t), NoReaders);
    pthread_t* WriterThreads = calloc(sizeof(pthread_t), NoWriters);

    unsigned rtid[ NoReaders ];
    for(size_t i = 0; i < NoReaders; i++)
    {
        rtid[ i ] = i;
        int ret = pthread_create(&ReaderThreads[ i ], NULL, readers_init, (void*) &rtid[i]);

        if(ret)
        {
            printf("Pthread_create() return with code: %d\n", ret);
            exit(EXIT_FAILURE);
        }
    }

    unsigned wtid[ NoWriters ];
    for(size_t i = 0; i < NoWriters; i++)
    {
        wtid[ i ] = i;
        int ret = pthread_create(&WriterThreads[ i ], NULL, writers_init, (void*) &wtid[i]);

        if(ret)
        {
            printf("Pthread_create() return with code: %d\n", ret);
            exit(EXIT_FAILURE);
        }
    }

    for(size_t i = 0; i < NoReaders; i++)
        pthread_join(ReaderThreads[ i ], NULL);

    for(size_t i = 0; i < NoWriters; i++)
        pthread_join(WriterThreads[ i ], NULL);

    pthread_cond_destroy(&(lock.cond));
    pthread_mutex_destroy(&(lock.mutex));
    return 0;
}