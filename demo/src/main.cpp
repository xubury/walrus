#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern "C" int wasm_main(int argc, char *argv[])
{
    printf("This is a string\n");
    printf("Hello world, argc:%d, argv[0]:%s\n", argc, argv[0]);
    printf("sinf:%f\n", sinf(1.0));
    char *ptr = (char *)malloc(100);
    ptr[0] = 122;
    printf("malloc:0x%lx ptr[0]:%d\n", (intptr_t)ptr, ptr[0]);
    return 0;
}
