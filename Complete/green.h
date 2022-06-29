#include <ucontext.h>

#ifndef GREEN_H
#define GREEN_H

typedef struct green_t {
    ucontext_t *context;
    void *(*fun)(void *);
    void *arg;
    struct green_t *next;
    struct green_t *join;
    void *retval;
    int zombie;
} green_t;

/*typedef struct queue_t {
    struct green_t *f, *r;    
} queue_t;*/


typedef struct node_t {
    green_t *curr;
    struct node_t *next;
} node_t;

typedef struct queue_t {
    volatile int taken;
    struct node_t *f, *r;    
} queue_t;

int green_create(green_t *thread, void*(*fun)(void*), void *arg);
int green_yield();
int green_join(green_t *thread, void **res);

void green_cond_wait();
void green_cond_signal();

int green_mutex_init();
int green_mutex_lock();
int green_mutex_unlock();

void enqueue(queue_t *head, green_t *curr);
green_t *dequeue(queue_t *head);
queue_t * createQueue();

#endif