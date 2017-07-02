#ifndef SORTED_LIST_H_
#define SORTED_LIST_H_

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    void *data;
    double score;
    void *prev;
    void *next;
} list_item_t;
#define LIST_ITEM_INIT (list_item_t){NULL, 0, 0}

typedef struct {
    list_item_t *first;
    list_item_t *last;
    size_t num_items;
    size_t max_num_items;
    bool max;
    void (*free_data)(void *data);
} list_t;
#define LIST_INIT (list_t){NULL, NULL, 0, 0, false, NULL}

int add_item(list_t *list, double score, void *data, size_t len);
list_item_t *create_item(double score, void *data, size_t len,
        list_item_t *prev, list_item_t *next);
int free_list(list_t *list);

#endif // SORTED_LIST_H_
