#include <stdio.h>
#include <goconc.h>

void print_A(void* arg) {
    printf("I am goroutine A!\n");
}


int main() {
    gc_go(print_A, nullptr);
    gc_run();

    return 0;
}
