#include <core/list.h>

#include <stdlib.h>

void walrus_list_free1(Walrus_List *list)
{
    free(list);
}

static void walrus_list_free_chain(Walrus_List *list)
{
    while (list) {
        walrus_list_free1(list);
        list = list->next;
    }
}

static Walrus_List *walrus_list_remove_link(Walrus_List *list, Walrus_List *link)
{
    if (link == NULL) {
        return list;
    }

    if (link->prev) {
        if (link->prev->next == link) {
            link->prev->next = link->next;
        }
        else {
            // warning corruputed link list
        }
    }
    if (link->next) {
        if (link->next->prev == link) {
            link->next->prev = link->prev;
        }
        else {
            // warning corruputed link list
        }
    }

    if (link == list) {
        list = list->next;
    }

    link->prev = NULL;
    link->next = NULL;

    return list;
}

//////////////////////

Walrus_List *walrus_list_alloc(void)
{
    Walrus_List *new = (Walrus_List *)malloc(sizeof(Walrus_List));
    new->data = NULL;
    new->next = NULL;
    new->prev = NULL;
    return new;
}

void walrus_list_free(Walrus_List *list)
{
    walrus_list_free_chain(list);
}

Walrus_List *walrus_list_last(Walrus_List *list)
{
    if (list) {
        while (list->next) list = list->next;
    }
    return list;
}

Walrus_List *walrus_list_append(Walrus_List *list, void *data)
{
    Walrus_List *new_list;
    Walrus_List *prev;

    new_list       = walrus_list_alloc();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        prev           = walrus_list_last(list);
        prev->next     = new_list;
        new_list->prev = prev;

        return list;
    }
    else {
        new_list->prev = NULL;

        return new_list;
    }
}

Walrus_List *walrus_list_remove(Walrus_List *list, void *data)
{
    Walrus_List *tmp;
    tmp = list;
    while (tmp) {
        if (tmp->data != data) {
            tmp = tmp->next;
        }
        else {
            list = walrus_list_remove_link(list, tmp);
            walrus_list_free1(tmp);

            break;
        }
    }
    return list;
}
