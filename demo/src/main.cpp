#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

struct Foo {
    Foo()
    {
        printf("Foo ctor\n");
    }
    ~Foo()
    {
        printf("Foo dtor\n");
    }
};

extern "C" int main(int argc, char *argv[])
{
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    time_t s  = spec.tv_sec;
    long ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }

    printf("Current time: %lld.%ld seconds since the Epoch\n", s, ms);

    printf("This is a string\n");
    printf("sinf:%f\n", sinf(1.0));
    char *ptr = (char *)malloc(100);
    ptr[0] = 122;
    ptr[1] = 123;
    ptr[2] = 124;
    printf("malloc:0x%lx\n", (intptr_t)ptr);
    Foo foo;
    for (int i = 0; i < 3; ++i) {
        printf("ptr[%d]:%d ", i, ptr[i]);
    }
    printf("\n");
    return 0;
}
