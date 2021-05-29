#pragma ONCE
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct node
{
    struct node* next;
    unsigned value;
} node_t;


typedef struct queue
{
    node_t* head;
} queue_t;

void queue_add(pthread_mutex_t*, queue_t*, unsigned);
void queue_remove(pthread_mutex_t*, queue_t*, unsigned);
bool queue_empty(queue_t*);

