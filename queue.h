#pragma ONCE
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

#define buff_size 500000000

typedef struct node
{
    struct node* next;
    unsigned value;
} node_t;

typedef struct 
{
    char buffer[ buff_size ];
    atomic_uint writers;
    atomic_uint readers;
} library;

typedef struct queue
{
    node_t* head;
    atomic_uint size;
} queue_t;

void queue_add_read_thread(pthread_mutex_t**, queue_t*, pthread_cond_t**, library*, unsigned);
void queue_add(pthread_mutex_t**, queue_t*, unsigned);
void queue_remove(pthread_mutex_t**, queue_t*, unsigned);
bool queue_empty(queue_t*);

