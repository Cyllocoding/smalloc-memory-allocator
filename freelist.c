#include <stdio.h>
#include "freelist.h"

#define BLOCK_END(b) ((char *)(b) + HDR_SIZE + (size_t)(b)->size)

void fl_init(common_header_t **head, void *mem, size_t mem_size)
{
    common_header_t *b = (common_header_t *)mem;
    b->size = (int)(mem_size - HDR_SIZE);
    b->next = NULL;
    *head = b;
}

common_header_t *fl_find_first_fit(common_header_t *head, size_t n,
                                   common_header_t **prev, size_t *scanned)
{
    common_header_t *p = NULL, *cur = head;
    while (cur) {
        (*scanned)++;
        if ((size_t)cur->size >= n) { *prev = p; return cur; }
        p = cur;
        cur = cur->next;
    }
    return NULL;
}

common_header_t *fl_find_best_fit(common_header_t *head, size_t n,
                                  common_header_t **prev, size_t *scanned)
{
    common_header_t *p = NULL, *cur = head, *best = NULL, *best_prev = NULL;
    while (cur) {
        (*scanned)++;
        if ((size_t)cur->size >= n && (!best || cur->size < best->size)) {
            best = cur;
            best_prev = p;
            if ((size_t)cur->size == n) break;
        }
        p = cur;
        cur = cur->next;
    }
    *prev = best_prev;
    return best;
}

static void fl_replace(common_header_t **head, common_header_t *prev,
                       common_header_t *with)
{
    if (prev) prev->next = with;
    else      *head = with;
}

void fl_take(common_header_t **head, common_header_t *prev,
             common_header_t *node, size_t n)
{
    if ((size_t)node->size >= n + HDR_SIZE + ALIGN) {
        common_header_t *rest = (common_header_t *)((char *)node + HDR_SIZE + n);
        rest->size = node->size - (int)(n + HDR_SIZE);
        rest->next = node->next;
        node->size = (int)n;
        fl_replace(head, prev, rest);
    } else {
        fl_replace(head, prev, node->next);
    }
    node->next = NULL;
}

void fl_insert_head(common_header_t **head, common_header_t *node)
{
    node->next = *head;
    *head = node;
}

void fl_insert_ordered(common_header_t **head, common_header_t *node,
                       common_header_t **prev_out)
{
    common_header_t *p = NULL, *cur = *head;
    while (cur && cur < node) { p = cur; cur = cur->next; }
    node->next = cur;
    if (p) p->next = node;
    else   *head = node;
    *prev_out = p;
}

void fl_merge(common_header_t *prev, common_header_t *node)
{
    common_header_t *next = node->next;
    if (next && BLOCK_END(node) == (char *)next) {
        node->size += (int)(HDR_SIZE + (size_t)next->size);
        node->next = next->next;
    }
    if (prev && BLOCK_END(prev) == (char *)node) {
        prev->size += (int)(HDR_SIZE + (size_t)node->size);
        prev->next = node->next;
    }
}

void fl_dump(const common_header_t *head)
{
    for (const common_header_t *cur = head; cur; cur = cur->next)
        printf("[%d]%s", cur->size, cur->next ? " -> " : "");
    printf("\n");
}

size_t fl_total_free(const common_header_t *head)
{
    size_t total = 0;
    for (const common_header_t *cur = head; cur; cur = cur->next)
        total += (size_t)cur->size;
    return total;
}

void fl_stats(const common_header_t *head, size_t *n, size_t *f, size_t *l)
{
    *n = *f = *l = 0;
    for (const common_header_t *cur = head; cur; cur = cur->next) {
        (*n)++;
        *f += (size_t)cur->size;
        if ((size_t)cur->size > *l) *l = (size_t)cur->size;
    }
}
