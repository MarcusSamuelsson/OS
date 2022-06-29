#include <stdio.h>
#include <unistd.h>
#include "green.h"

int flag = 0;
green_cond_t cond;

void *test(void *arg) {
    int id = *(int*)arg;
    int loop = 6;

    while(loop > 0) {
        if(flag == id){
            printf("thread %d : %d\n", id, loop);
            loop--;
            flag = (id+1) % 2;
            green_cond_signal(&cond);
        } else {
            green_cond_wait(&cond);
        }
    }
    //printf("daheq\n");
}

int main() {
    green_cond_init(&cond);
    
    green_t g0, g1;

    int a0 = 0;
    int a1 = 1;

    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);
    //printf("lol\n");

    green_join(&g0, NULL);
    //printf("yahoo\n");
    green_join(&g1, NULL);

    printf("done\n");

    return 0;
}