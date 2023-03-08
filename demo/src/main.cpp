#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

int main()
{
    printf("This is a string\n");
    // printf("Hello world, argc:%d, argv[0]:%s\n", argc, argv[0]);
    printf("sinf:%f\n", sinf(1.0));
    printf("Hello world\n");
    void *ptr = malloc(123);
    printf("malloc:%ld\n", (intptr_t)ptr);
    free(ptr);
    return 0;
}
