#include "green.h"
#include <stdio.h>
#include <stdlib.h>

volatile int count = 0;
void* test_mutex(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 100000; i++) {
        green_mutex_lock();
        count++;
        green_mutex_unlock();
    }
}

int main(int argc, char const *argv[]) {
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;

    green_mutex_init();

    green_create(&g0, test_mutex, &a0);
    green_create(&g1, test_mutex, &a1);
    green_join(&g0, NULL);
    green_join(&g1, NULL);

    printf("result: %d\n", count);
    printf("done\n");
    return 0;
}