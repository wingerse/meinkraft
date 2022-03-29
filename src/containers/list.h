/*
 * Generic list
 * LIST_DECLARE defines the struct and declares the functions.
 * LIST_DEFINE defines the functions.
 * Do not put semi-colon after either of the macros.
 * After declaration for type T, generic list_T and generic list_T_functionname will be available.
 */

#pragma once

#include <stddef.h>
#include <stdlib.h>

#define LIST_INITIAL_CAPACITY 4

#define LIST_DECLARE(T) \
typedef struct list_##T {\
    T      *data;\
    size_t len;\
    size_t cap;\
} list_##T;\
\
void list_##T##_init_custom(list_##T *l, size_t initial_cap);\
void list_##T##_init(list_##T *l);\
T   *list_##T##_add(list_##T *l);\
void list_##T##_clear(list_##T *l);\
void list_##T##_destroy(list_##T *l);

#define LIST_DEFINE(T) \
void list_##T##_init_custom(list_##T *l, size_t initial_cap)\
{\
    l->len = 0;\
    size_t cap = 1;\
    while (cap < initial_cap) {\
        cap <<= 1;\
    }\
    l->cap = cap;\
    l->data = malloc(l->cap * sizeof(T));\
}\
\
void list_##T##_init(list_##T *l)\
{\
    list_##T##_init_custom(l, LIST_INITIAL_CAPACITY);\
}\
\
T * list_##T##_add(list_##T *l)\
{\
    if (l->len == l->cap) {\
        l->cap <<= 1;\
        l->data = realloc(l->data, l->cap * sizeof(T));\
    }\
    return &l->data[l->len++];\
}\
\
void list_##T##_clear(list_##T *l)\
{\
    l->len = 0;\
}\
\
void list_##T##_destroy(list_##T *l)\
{\
    free(l->data);\
}
