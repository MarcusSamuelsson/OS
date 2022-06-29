#include "green_ts.h"
#include <stdio.h>
#include <stdlib.h>

char buf[15] = "thread  \n done\n";
int running_id;
void* test_success(void* arg) {
    int id = *(int*)arg;
    running_id = id;
    buf[7] = (char) (id+48);
    write(1, buf, 9);
    while (running_id == 0) {} //wait for 2nd thread to complete
    buf[7] = (char) (id+48);
    buf[8] = 0;
    write(1, buf, 15);
}

int main(int argc, char const *argv[]) {
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;
    green_create(&g0, test_success, &a0);
    green_create(&g1, test_success, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    printf("done\n");
    return 0;
}
