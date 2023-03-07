#define __wasi__

#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern uint8_t memory;

extern "C" void run() {
  const char *str = "hello world";
  size_t len = strlen(str);
  printf("hello world\n");
}
