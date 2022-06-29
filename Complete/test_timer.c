#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "green.h"

char buffer[15] = "thread \n done\n";
int curr_id;

void *test(void *arg) {
    int i = *(int*) arg;
    curr_id = i;
    buffer[7] = (char) (i+48);
    write(1, buffer, 9);
    while(curr_id == 0) {}
    buffer[7] = (char) (i+48);
    buffer[8] = 0;
    write(1, buffer, 15);
}

int main() {
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