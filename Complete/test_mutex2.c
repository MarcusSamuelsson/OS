#include "green.h"
#include <stdio.h>

int flag = 0;
void* test(void* arg) {
    int id = *(int*)arg;
    int loop = 10000;
    while (loop > 0) {
        green_mutex_lock();
        while (flag != id)
            green_cond_wait();

        printf("thread %d: %d\n", id, loop);

        flag = (id + 1) % 2;
        green_cond_signal();
        green_mutex_unlock();
        loop--;
    }
}

int main(int argc, char const *argv[]) {
    green_mutex_init();
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;

    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    

    printf("done\n");
    return 0;
}