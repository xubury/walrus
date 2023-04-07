#pragma once

#include <core/type.h>

typedef struct _List List;

struct _List {
    void *data;
    List *next;
    List *prev;
};

// Allocate a double linked list
List *list_alloc(void);

// Free a double linked list
void list_free(List *list);

// Free one node from linked list
void list_free1(List *list);

// Return the last element of linked list
List *list_last(List *list);

// Append an element to the last of a linked list
List *list_append(List *list, void *data);

// Remove an element from the last of a linked list
List *list_remove(List *list, void *data);
