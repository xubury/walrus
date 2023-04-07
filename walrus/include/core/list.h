#pragma once

#include <core/type.h>

typedef struct _Walrus_List Walrus_List;

struct _Walrus_List {
    void        *data;
    Walrus_List *next;
    Walrus_List *prev;
};

// Allocate a double linked list
Walrus_List *walrus_list_alloc(void);

// Free a double linked list
void walrus_list_free(Walrus_List *list);

// Free one node from linked list
void walrus_list_free1(Walrus_List *list);

// Return the last element of linked list
Walrus_List *walrus_list_last(Walrus_List *list);

// Append an element to the last of a linked list
Walrus_List *walrus_list_append(Walrus_List *list, void *data);

// Remove an element from the last of a linked list
Walrus_List *walrus_list_remove(Walrus_List *list, void *data);
