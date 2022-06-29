#include <ucontext.h>

typedef struct green_t {
    ucontext_t* context;
    void* (*fun)(void*);    //a function pointer.
    void* arg;  //pointer to the functions args
    struct green_t* next;   //pointer to next thread in rdy queue
    struct green_t* join;   //pointer to a thread waiting on this one
    void* retval;
    int zombie;
} green_t;

typedef struct green_cond_t {
    struct green_t* first;   //list of suspended threads
    struct green_t* last;
} green_cond_t;

int green_create(green_t* thread, void* (*fun)(void*), void* arg);
int green_yield();
int green_join(green_t* thread, void** val);
void green_cond_init(green_cond_t* cond);
void green_cond_wait(green_cond_t* cond);
void green_cond_signal(green_cond_t* cond);
