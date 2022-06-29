#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "green.h"

volatile int count = 0;
int loop = 10000000;

void *test(void *arg) {
    for(int i = 0; i < loop; i++)
        count++;
}

int main() {
    green_t g0, g1;

    int a0 = 0;
    int a1 = 1;

    printf("increments: %d\n", loop);

    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    printf("Result: %d\n", count);

    printf("done\n");
    
    return 0;
}