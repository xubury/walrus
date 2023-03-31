#pragma once

#include <type.h>

typedef struct _List List;

struct _List {
    u8   *data;
    List *next;
    List *prev;
};

List *list_alloc(void);

void list_free(List *list);

List *list_push(List *list, void *data);

List *list_pop(List *list);
