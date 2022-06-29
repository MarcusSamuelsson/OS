#include <stdlib.h>
#include <stdio.h>
#include "green.h"
#include <assert.h>
#include <ucontext.h>

#define FALSE 0
#define TRUE 1
#define STACK_SIZE 4096

//global context to store initial running process
static ucontext_t main_cntx = {0};
//thread
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};

static green_t* running = &main_green;

//function called when program is loaded
static void init() __attribute__((constructor));

void init() {
    getcontext(&main_cntx);
}

green_t* rdyq = NULL;
green_t* rdyq_last = NULL;
void enqueue(green_t* thread) {
    if (rdyq == NULL) 
        rdyq = thread;
    else
        rdyq_last->next = thread;

    rdyq_last = thread;
    rdyq_last->next = NULL;
}
green_t* dequeue() {
    if (rdyq == NULL)
        return NULL;

    green_t* first = rdyq;

    if (rdyq == rdyq_last) {
        rdyq = NULL;
        rdyq_last = NULL;
    }
    else
        rdyq = rdyq->next;

    first->next = NULL;
    return first;
}

void green_thread() {
    green_t* this = running;

    void* result = (*this->fun)(this->arg);

    if (this->join != NULL)
        enqueue(this->join);

    this->retval = result;
    this->zombie = TRUE;

    green_t* next = dequeue();
    running = next;
    setcontext(next->context);
}

int green_yield() {
    green_t* susp = running;
    enqueue(susp);

    green_t* next = dequeue();
    running = next;
    swapcontext(susp->context, next->context);
    return 0;
}

int green_create(green_t* new, void* (*fun)(void*), void* arg) {
    ucontext_t* cntx = (ucontext_t*) malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void* stack = malloc(STACK_SIZE);

    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;
    makecontext(cntx, green_thread, 0);

    new->context = cntx;
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->retval = NULL;
    new->zombie = FALSE;

    enqueue(new);
    return 0;
}

int green_join(green_t* thread, void** res) {
    if (!thread->zombie) {
        green_t* susp = running;
        thread->join = susp;

        green_t* next = dequeue();
        if (next != NULL) {
            running = next;
            swapcontext(susp->context, next->context);
        }
    }

    res = thread->retval;
    free(thread->context);
    return 0;
}

void green_cond_init(green_cond_t* cond) {
    cond->first = NULL;
    cond->last = NULL;
}
void green_cond_wait(green_cond_t* cond) {
    green_t* susp = running;
    if (cond->first == NULL) {
        cond->first = susp;
        cond->first->next = NULL;
    }
    else
        cond->last->next = susp;

    cond->last = susp;
    cond->last->next = NULL;
    green_t* next = dequeue();
    running = next;
    swapcontext(susp->context, next->context);
}
void green_cond_signal(green_cond_t* cond) {
    if (cond->first != NULL) {
        enqueue(cond->first);
        cond->first = cond->first->next;

        if (cond->first == NULL)
            cond->last = NULL;
    }
}

void timer_handler(int sig) {
    green_t* susp = running;
    enqueue(susp);

    green_t* next = dequeue();
    running = next;
    swapcontext(susp->context, next->context);
}
