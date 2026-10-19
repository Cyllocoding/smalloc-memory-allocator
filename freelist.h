#ifndef FREELIST_H
#define FREELIST_H

#include <stddef.h>

typedef struct __common_header_t {
    int size;                          /* payload bytes, header excluded */
    struct __common_header_t *next;    /* next free block; NULL when allocated */
} common_header_t;

#define HDR_SIZE     sizeof(common_header_t)
#define ALIGN        16
#define ROUND_UP(n)  (((n) + ALIGN - 1) & ~((size_t)ALIGN - 1))

void fl_init(common_header_t **head, void *mem, size_t mem_size);

common_header_t *fl_find_first_fit(common_header_t *head, size_t n,
                                   common_header_t **prev, size_t *scanned);
common_header_t *fl_find_best_fit(common_header_t *head, size_t n,
                                  common_header_t **prev, size_t *scanned);

void fl_take(common_header_t **head, common_header_t *prev,
             common_header_t *node, size_t n);

void fl_insert_head(common_header_t **head, common_header_t *node);
void fl_insert_ordered(common_header_t **head, common_header_t *node,
                       common_header_t **prev_out);
void fl_merge(common_header_t *prev, common_header_t *node);

void   fl_dump(const common_header_t *head);
size_t fl_total_free(const common_header_t *head);
void   fl_stats(const common_header_t *head, size_t *n, size_t *f, size_t *l);

#endif
