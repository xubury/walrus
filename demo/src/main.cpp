#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

extern "C" int wasm_main(int argc, char *argv[])
{
    printf("This is a string\n");
    printf("Hello world, argc:%d, argv[0]:%s\n", argc, argv[0]);
    printf("sinf:%f\n", sinf(1.0));
    void *ptr = malloc(100);
    printf("malloc:0x%lx\n", (intptr_t)ptr);
    return 0;
}
