#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ucontext.h>
#include <assert.h>
#include "green.h"

#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>

#define PERIOD 100

#define FALSE 0
#define TRUE 1
#define STACK_SIZE 4096

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};
static green_t *running = &main_green;
static queue_t *readyQueue, *condQueue, *mutexQueue;

static sigset_t block;

void timer_handler(int);

static void init() __attribute__((constructor));

void init() {
    getcontext(&main_cntx);

    readyQueue = createQueue();
    mutexQueue = createQueue();
    condQueue = createQueue();

    sigemptyset(&block);
    sigaddset(&block, SIGVTALRM);

    struct sigaction act = {0};
    struct timeval interval;
    struct itimerval period;

    act.sa_handler = timer_handler;
    assert(sigaction(SIGVTALRM, &act, NULL) == 0);
    interval.tv_sec = 0;
    interval.tv_usec = PERIOD;
    period.it_interval = interval;
    period.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &period, NULL);
}

void timer_handler(int sig) {
    //sigprocmask(SIG_BLOCK, &block , NULL);

    /*green_t* susp = running;
    enqueue(readyQueue, susp);

    green_t* next = dequeue(readyQueue);
    running = next;
    swapcontext(susp->context, next->context);*/
    //sigprocmask(SIG_UNBLOCK, &block , NULL);
}

void green_thread() {
    green_t *this = running;

    void *result = (*this->fun)(this->arg);

    //place waiting (joining) thread in ready queue
    if(this->join != NULL)
        enqueue(readyQueue, this->join);
    
    //save result of execution
    this->retval = result;
    
    //we're a zombie
    this->zombie = TRUE;
    
    //find the next thread to run
    
    green_t *next = dequeue(readyQueue);
    running = next;
    //this->next = NULL;
    setcontext(next->context);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {
    ucontext_t *cntx = (ucontext_t*)malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void *stack = malloc(STACK_SIZE);

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

    //add new to the ready queue
    enqueue(readyQueue, new);

    return 0;
}

int green_yield() {
    //sigprocmask(SIG_BLOCK, &block , NULL);
    green_t *susp = running;
    
    //add susp to ready queue
    enqueue(readyQueue, susp);

    //select the next thread for execution
    green_t *next = dequeue(readyQueue);

    running = next;

    //susp->next = NULL;
    //write(1, "hello world\n", 12);

    swapcontext(susp->context, next->context);

    //sigprocmask(SIG_UNBLOCK, &block , NULL);
    return 0;
}

int green_join(green_t *thread, void **res) {
    //sigprocmask(SIG_BLOCK, &block , NULL);
    if(!thread->zombie) {
        green_t *susp = running;

        //add as joining thread
        thread->join = susp;

        //select the next thread for execution
        green_t *next = dequeue(readyQueue);
        if(next != NULL) {
            running = next;
            swapcontext(susp->context, running->context);
        }
    }
    //collect result
    res = thread->retval;
    
    //free context
    free(thread->context);

    //sigprocmask(SIG_UNBLOCK, &block , NULL);
    return 0;
}

void green_cond_wait() {
    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t* susp = running;
    enqueue(condQueue, susp);

    //unlock
    if(mutexQueue != NULL) {
        if(mutexQueue->f != NULL) {
            green_t* susp = dequeue(mutexQueue);
            enqueue(readyQueue, susp);
        }
        else
            mutexQueue->taken = FALSE;
    }

    //schedule next thread
    green_t* next = dequeue(readyQueue);
    running = next;
    swapcontext(susp->context, next->context);

    //when signaled that cond is true, continue here
    //lock
    if(mutexQueue != NULL) {
        susp = running;
        if(mutexQueue->taken) {
            enqueue(mutexQueue, susp);

            green_t* next = dequeue(readyQueue);
            running = next;
            swapcontext(susp->context, next->context);
        }
        else
            mutexQueue->taken = TRUE;
    }

    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

void green_cond_signal() {
    sigprocmask(SIG_BLOCK, &block, NULL);

    if (condQueue->f != NULL) {
        enqueue(readyQueue, dequeue(condQueue));
    }
    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_mutex_init() {
    sigprocmask(SIG_BLOCK, &block, NULL);
    mutexQueue->f = FALSE;
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_mutex_lock() {
    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t* susp = running;
    if(mutexQueue->taken) {
        enqueue(mutexQueue, susp);

        green_t* next = dequeue(readyQueue);
        running = next;
        swapcontext(susp->context, next->context);
    }
    else
        mutexQueue->taken = TRUE;

    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_mutex_unlock() {
    sigprocmask(SIG_BLOCK, &block, NULL);
    if(mutexQueue->f != NULL) {
        green_t* susp = dequeue(mutexQueue);
        enqueue(readyQueue, susp);
    }
    else
        mutexQueue->taken = FALSE;

    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

/*void green_cond_init(green_cond_t *c) {
    //c = (green_cond_t *) malloc(sizeof(green_cond_t));
    c->q = (queue_t*) malloc(sizeof(queue_t));
    c->q->f = NULL;
    c->q->r = NULL;
}

void green_cond_wait(green_cond_t *c) {
    //sigprocmask(SIG_BLOCK, &block , NULL);
    green_t *susp = running;
    green_t *next = susp->next;

    if(c->q->r != NULL)
        c->q->r->next = susp;
    
    if(c->q->f == NULL)
        c->q->f = susp;

    c->q->r = susp;

    //c->q->r->next = NULL;

    running = next;
    //susp->next = NULL;
    if(next != NULL)
        swapcontext(susp->context, running->context);
    //sigprocmask(SIG_UNBLOCK, &block , NULL);
}

void green_cond_signal(green_cond_t *c) {
    if(c->q->f == NULL)
        return;

    ////sigprocmask(SIG_BLOCK, &block , NULL);
    
    if(rQueueRear != NULL)
        rQueueRear->next = c->q->f;

    if(running == NULL)
        running = c->q->f;

    rQueueRear = c->q->f;

    //printf("next: %p\n", c->q->f->next);

    green_t *next = c->q->f->next;
    c->q->f->next = NULL;
    //printf("next2: %p\n", next);
    c->q->f = next;
    //printf("next3: %p\n", c->q->f);
    //printf("next4: %p\n", c->q->f->next);
    ////sigprocmask(SIG_UNBLOCK, &block , NULL);
}*/