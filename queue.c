#include "queue.h"
#include <stdio.h>

bool queue_empty(queue_t* q)    {
    return q->head == NULL;     }

void queue_add_read_thread(pthread_mutex_t** queue_lock, queue_t* q, pthread_cond_t** allow_add, library* lib, unsigned value)
{
    pthread_mutex_lock(*queue_lock);

    while(lib->writers == 0)
        pthread_cond_wait(*allow_add, *queue_lock);

    queue_add(queue_lock, q, value);
    pthread_mutex_unlock(*queue_lock);
}

void queue_add(pthread_mutex_t** queue_lock, queue_t* q, unsigned value)
{
    pthread_mutex_lock(*queue_lock);

    node_t* node = (node_t*) calloc(sizeof(node_t), 1);
    node->value = value;

    if(!q->head)
        q->head = node;
    else
    {
        node_t* n = q->head;

        while (n->next != NULL)
            n = n->next;

        n->next = node;
    }

    q->size++;
    pthread_mutex_unlock(*queue_lock);
}

void queue_remove(pthread_mutex_t** queue_lock, queue_t* q, unsigned value)
{
    pthread_mutex_lock(*queue_lock);

    node_t* node = q->head;
    node_t* prev = q->head;

    if(!q->head)
        return;

    while(node)
    {
        if(node->value == value)
        {
            if(prev == node)
                q->head =  node->next;
            else
                prev->next = node->next;
            
            free(node);
            q->size--;
            break;
        }
        
        prev = node;
        node = node->next;
    }

    pthread_mutex_unlock(*queue_lock);
}