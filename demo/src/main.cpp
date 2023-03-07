#include <stdint.h>
#include <string.h>

extern "C" void console_log(uint32_t value);
extern uint8_t memory;

extern "C" void run()
{
    console_log(123);
    char *ptr = (char *)&memory;
    const char *str = "hello world";
    for (uint8_t i = 0; i < strlen(str); ++i) {
        ptr[i] = str[i];
    }
}
