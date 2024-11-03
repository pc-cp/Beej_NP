#include "../../recipes/thread/Thread.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

void f1() {
    errno = 1;
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "%s\n", strerror(errno));
}

void f2() {
    errno = 2;
    fprintf(stderr, "Value of errno: %d\n", errno);
    fprintf(stderr, "%s\n", strerror(errno));
}

int main() {
    muduo::Thread t1(f1);
    muduo::Thread t2(f2);

    t1.start();
    t2.start();

    t1.join();
    t2.join();

    return 0;
}