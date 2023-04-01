#include <list.h>
#include <stdlib.h>

void list_free1(List *list)
{
    free(list);
}

static void list_free_chain(List *list)
{
    while (list) {
        list_free1(list);
        list = list->next;
    }
}

static List *list_remove_link(List *list, List *link)
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

List *list_alloc(void)
{
    List *new = (List *)malloc(sizeof(List));
    new->data = NULL;
    new->next = NULL;
    new->prev = NULL;
    return new;
}

void list_free(List *list)
{
    list_free_chain(list);
}

List *list_last(List *list)
{
    if (list) {
        while (list->next) list = list->next;
    }
    return list;
}

List *list_append(List *list, void *data)
{
    List *new_list;
    List *prev;

    new_list       = list_alloc();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        prev           = list_last(list);
        prev->next     = new_list;
        new_list->prev = prev;

        return list;
    }
    else {
        new_list->prev = NULL;

        return new_list;
    }
}

List *list_remove(List *list, void *data)
{
    List *tmp;
    tmp = list;
    while (tmp) {
        if (tmp->data != data) {
            tmp = tmp->next;
        }
        else {
            list = list_remove_link(list, tmp);
            list_free1(tmp);

            break;
        }
    }
    return list;
}
