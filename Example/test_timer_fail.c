#include "green_ts.h"
#include <stdio.h>
#include <stdlib.h>

volatile int count = 0;
int loop = 100000;
void* test_fail(void* arg) {
    for (int i = 0; i < loop; i++)
        count++;
}

int main(int argc, char const *argv[]) {
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;

    printf("increments: %d\n", loop);

    green_create(&g0, test_fail, &a0);
    green_create(&g1, test_fail, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    printf("result: %d\n", count);
    printf("done\n");
    return 0;
}
