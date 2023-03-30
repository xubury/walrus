#include <input.h>
#include <event.h>

#include <stdio.h>

void __on_mouse_move(i32 x, i32 y, i32 xrel, i32 yrel, i32 mod)
{
    printf("on mouse move x:%d y:%d xrel:%d yrel:%d mod:%d\n", x, y, xrel, yrel, mod);
}

void __on_mouse_click(i8 btn, i32 x, i32 y, i32 xrel, i32 yrel, i32 mod)
{
    printf("on mouse click btn:%d x:%d y:%d mod:%d xrel:%d yrel:%d\n", btn, x, y, mod, xrel, yrel);
}

void __on_mouse_up(i8 btn, i32 x, i32 y, i32 xrel, i32 yrel, i32 mod)
{
    printf("on mouse up btn:%d x:%d y:%d mod:%d xrel:%d yrel:%d\n", btn, x, y, mod, xrel, yrel);
}

void __on_mouse_down(i8 btn, i32 x, i32 y, i32 xrel, i32 yrel, i32 mod)
{
    printf("on mouse down btn:%d x:%d y:%d mod:%d xrel:%d yrel:%d\n", btn, x, y, mod, xrel, yrel);
}

