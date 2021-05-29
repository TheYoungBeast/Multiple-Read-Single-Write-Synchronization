#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "queue.h"

#define buff_size 15000000

pthread_mutex_t WriteQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ReadQueueLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t liblock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t WriteCond = PTHREAD_COND_INITIALIZER;

queue_t ReadThreadQueue = { NULL };
queue_t WriteThreadQueue = { NULL };

typedef struct 
{
    char buffer[ buff_size ];
    atomic_uint writers;
    atomic_uint readers;
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
    queue_add(&ReadQueueLock, &ReadThreadQueue, id);

    while(true) 
    {
        while (Library.writers != 0 || queue_empty(&WriteThreadQueue) != true)
            pthread_cond_broadcast(&WriteCond); // broadcast Writing if it took over (case of signle write thread in queue)

        ++Library.readers;
        print_library_state(id);

        for(size_t i = 0; i < buff_size; i++)
        {
            char c = Library.buffer[ i ];
        }

        --Library.readers;
        print_library_state(id);
        queue_remove(&ReadQueueLock, &ReadThreadQueue, id);

        //if(queue_empty(&ReadThreadQueue))
            //pthread_cond_broadcast(&ReadQueueEmpty);

        queue_add(&ReadQueueLock, &ReadThreadQueue, id);
    }
}

void* writers_init(void* tid)
{
    unsigned id = *((unsigned*) tid)+1;
    queue_add(&WriteQueueLock, &WriteThreadQueue, id);
    srandom(time(NULL));

    while(true) 
    {
        while (queue_empty(&WriteThreadQueue) || id != WriteThreadQueue.head->value)
            continue;
        
        pthread_mutex_lock(&liblock);

        while (Library.writers != 0 || Library.readers != 0)
            pthread_cond_wait(&WriteCond, &liblock);
        
        ++Library.writers;
        print_library_state(id);

        size_t pos = random() % (buff_size/2); // update at least half of the buffer
        for(size_t i = pos; i < buff_size; i++)
            Library.buffer[ i ] = (char) (random() % 255);

        --Library.writers;
        print_library_state(id);

        queue_remove(&WriteQueueLock, &WriteThreadQueue, id);
        queue_add(&WriteQueueLock, &WriteThreadQueue, id);
        pthread_mutex_unlock(&liblock);
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

    for(size_t i = 0; i < NoReaders; i++)
        pthread_join(ReaderThreads[ i ], NULL);

    for(size_t i = 0; i < NoWriters; i++)
        pthread_join(WriterThreads[ i ], NULL);

    pthread_cond_destroy(&WriteCond);
    pthread_mutex_destroy(&liblock);
    return 0;
}