#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdatomic.h>
#define __USE_GNU
#include <pthread.h>

#include "queue.h"

typedef enum last_thread
{
    READ_THREAD,
    WRITE_THREAD
} last_thread;

last_thread last_active_thread;

pthread_mutex_t WriteQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ReadQueueLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP; // RECURSIVE MUTEX  // GNU EXTENSION

pthread_mutex_t liblock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readliblock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t WriteCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t ReadCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t AllowAdd = PTHREAD_COND_INITIALIZER;

queue_t ReadThreadQueue = { NULL, 0 };
queue_t WriteThreadQueue = { NULL, 0 };

library Library = {{0}, 0, 0};

void print_library_state(unsigned id)
{
    //printf("\033[H\033[J");
    printf("[RQ:%d WQ:%d] \tr: %u \tw: %u \tthread: %u\n", ReadThreadQueue.size, WriteThreadQueue.size, Library.readers, Library.writers, id);
    fflush(stdout);
}

void* readers_init(void* tid)
{
    unsigned id = *((unsigned*) tid)+1;
    pthread_mutex_t* l = &ReadQueueLock;
    queue_add(&l, &ReadThreadQueue, id);

    while(true) 
    {
        pthread_mutex_lock(&readliblock);           // lock mutex

        while (Library.writers != 0 ||              // read-thread is not allowed to enter if there's any write-thread active
                queue_empty(&ReadThreadQueue) ||    // read-thread is not allowed to enter if target ThreadQueue is empty
                id != ReadThreadQueue.head->value)  // read-thread is not allowed to enter if its id doesn't match the position in the queue
            pthread_cond_wait(&ReadCond, &readliblock); // if one of the above occurred wait for the conditional variable signal

        pthread_cond_broadcast(&ReadCond);
        ++Library.readers;                          // increase number of active readers
        pthread_mutex_t* l = &ReadQueueLock;
        queue_remove(&l, &ReadThreadQueue, id);     // emove read-thread from the queue since it entered
        print_library_state(id);                    // print state of the library (includes both queues)
        last_active_thread = READ_THREAD;           // mark the last active thread in the lib
        pthread_mutex_unlock(&readliblock);         // unlock mutex to allow other read-threads to enter

        for(size_t i = 0; i < buff_size; i++)       // read whole buffer (primitive simulation of reading operations)
        {
            char c = Library.buffer[ i ];
        }

        pthread_mutex_lock(&readliblock);
        --Library.readers;                          // decrease number of active readers
        print_library_state(id);                    // print state of the library (includes both queues)
        pthread_mutex_unlock(&readliblock);

        if(queue_empty(&ReadThreadQueue) || !Library.readers)
            pthread_cond_broadcast(&WriteCond);     // broadcast the condition variable to allow write-thread to enter, all read-threads are done

        pthread_mutex_t* rl = &ReadQueueLock;
        pthread_cond_t* c = &AllowAdd;
        queue_add_read_thread(&rl, &ReadThreadQueue, &c, &Library, id); // add read-thread to the read-queue
    }
}

void* writers_init(void* tid)
{
    unsigned id = *((unsigned*) tid)+1;
    pthread_mutex_t* l = &WriteQueueLock;
    queue_add(&l, &WriteThreadQueue, id);

    while(true) 
    {
        pthread_mutex_lock(&liblock);               // lock mutex

        while (Library.writers != 0 ||              // write-thread is not allowed to enter if there's already another write-thread active
                Library.readers != 0 ||             // write-thread is not allowed to enter if there's any read-thread active
                last_active_thread == WRITE_THREAD||// write-thread is not allowed to enter if a last active thread was write-thread too
                queue_empty(&WriteThreadQueue) ||   // write-thread is not allowed to enter if target ThreadQueue is empty
                id != WriteThreadQueue.head->value) // write-thread is not allowed to enter if its id doesn't match the position in the queue
            pthread_cond_wait(&WriteCond, &liblock);// if one of the above occurred wait for the conditional variable signal

        ++Library.writers;                          // increase number of active writers
        pthread_mutex_t* l = &WriteQueueLock;
        queue_remove(&l, &WriteThreadQueue, id);    // remove write-thread from the queue since it entered
        print_library_state(id);                    // print state of the library (includes both queues)
        last_active_thread = WRITE_THREAD;          // mark the last active thread in the lib

        pthread_cond_broadcast(&AllowAdd);          // broadcast the condition variable to allow adding to read-queue

        size_t pos = lrand48() % (buff_size/2);     // update at least half of the buffer (primitive simulation of write operations) at least 0.25GB in this case
        for(size_t i = pos; i < buff_size; i++)
            Library.buffer[ i ] = (char) (lrand48() % 255);

        --Library.writers;                          // decrease number of active writers
        print_library_state(id);                    // print state of the library (includes both queues)

        pthread_mutex_unlock(&liblock);             // release the lock

        if(queue_empty(&WriteThreadQueue) || !Library.writers)
            pthread_cond_broadcast(&ReadCond);      // broadcast the condition variable to allow read-thread to enter now
        if(queue_empty(&ReadThreadQueue))           // if the read-queue is empty allow write after write situation [not used here]
            pthread_cond_broadcast(&WriteCond);
        
        pthread_mutex_t* wl = &WriteQueueLock;
        queue_add(&wl, &WriteThreadQueue, id);      // add write-thread to the write-queue
    }
}

int main(int argc, char** argv)                     // main is basic and self-explanatory
{
    if(argc < 3)
    {
        printf("Usage: ./rwp [Number_of_Readers] [Number_of_Writers]\n");
        return 0;
    }

    srand48(time(NULL)); // thread-safe random

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

    pthread_cond_destroy(&WriteCond);
    pthread_mutex_destroy(&liblock);
    return 0;
}