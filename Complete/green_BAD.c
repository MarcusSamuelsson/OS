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
static green_t *rQueueRear = NULL;

static sigset_t block;

void timer_handler(int);

static void init() __attribute__((constructor));

void init() {
    getcontext(&main_cntx);

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
    //write(1, "hello world\n", 12);
    if(sig == SIGALRM)
        green_yield();
}

void green_thread() {
    sigprocmask(SIG_BLOCK, &block , NULL);
    green_t *this = running;

    void *result = (*this->fun)(this->arg);

    //place waiting (joining) thread in ready queue
    for (green_t *i = running; i != NULL; i = i->next) {
        if(i->join != NULL) {
            rQueueRear->next = i->join;
            rQueueRear = i->join;
            i->join = NULL;
        }
    }
    
    //save result of execution
    this->retval = result;
    
    //we're a zombie
    this->zombie = TRUE;
    
    //find the next thread to run
    
    green_t *next = this->next;
    running = next;
    //this->next = NULL;
    setcontext(next->context);
    sigprocmask(SIG_UNBLOCK, &block , NULL);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {
    sigprocmask(SIG_BLOCK, &block , NULL);
    ucontext_t *cntx = (ucontext_t*)malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void *stack = malloc(STACK_SIZE);

    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;

    makecontext(cntx, (void (*)(void))green_thread, 0);

    new->context = cntx;
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->retval = NULL;
    new->zombie = FALSE;

    //add new to the ready queue
    if(rQueueRear == NULL) 
        running->next = new;
    else
        rQueueRear->next = new;
    
    rQueueRear = new;
    sigprocmask(SIG_UNBLOCK, &block , NULL);

    return 0;
}

int green_yield() {
    sigprocmask(SIG_BLOCK, &block , NULL);
    green_t *susp = running;
    
    //add susp to ready queue
    if(running != rQueueRear)
        rQueueRear->next = susp;

    rQueueRear = susp;

    //select the next thread for execution
    green_t *next = susp->next;

    running = next;

    //susp->next = NULL;
    //write(1, "hello world\n", 12);

    if(next != NULL)
        swapcontext(susp->context, running->context);
    sigprocmask(SIG_UNBLOCK, &block , NULL);
    return 0;
}

int green_join(green_t *thread, void **res) {
    sigprocmask(SIG_BLOCK, &block , NULL);
    if(thread->zombie == FALSE) {
        green_t *susp = running;

        //add as joining thread
        susp->join = thread;

        //select the next thread for execution
        green_t *next = susp->next;
        running = next;
        //susp->next = NULL;
        if(next != NULL)
            swapcontext(susp->context, running->context);
    }
    //collect result
    if(res != NULL){
        *res = thread->retval;
        thread->retval = NULL;
    }
    
    write(1, "hello world\n", 12);
    
    //free context
    munmap(thread->context->uc_stack.ss_sp, thread->context->uc_stack.ss_size);
    free(thread->context);
    thread->context = NULL;

    sigprocmask(SIG_UNBLOCK, &block , NULL);
    return 0;
}

void green_cond_init(green_cond_t *c) {
    //c = (green_cond_t *) malloc(sizeof(green_cond_t));
    c->q = (queue_t*) malloc(sizeof(queue_t));
    c->q->f = NULL;
    c->q->r = NULL;
}

void green_cond_wait(green_cond_t *c) {
    sigprocmask(SIG_BLOCK, &block , NULL);
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
    sigprocmask(SIG_UNBLOCK, &block , NULL);
}

void green_cond_signal(green_cond_t *c) {
    if(c->q->f == NULL)
        return;

    //sigprocmask(SIG_BLOCK, &block , NULL);
    
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
    //sigprocmask(SIG_UNBLOCK, &block , NULL);
}