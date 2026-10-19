/* Drop-in malloc/free built on smalloc/sfree. Build as a shared library
 * and load with LD_PRELOAD (Linux). Memory is capped at MEM_SIZE. */
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "allocator.h"
#include "freelist.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *malloc(size_t n)
{
    pthread_mutex_lock(&lock);
    void *p = smalloc(n ? n : 1);
    pthread_mutex_unlock(&lock);
    return p;
}

void free(void *p)
{
    pthread_mutex_lock(&lock);
    sfree(p);
    pthread_mutex_unlock(&lock);
}

void *calloc(size_t count, size_t size)
{
    if (size && count > SIZE_MAX / size) return NULL;
    size_t n = count * size;
    void *p = malloc(n);
    if (p) memset(p, 0, n);
    return p;
}

void *realloc(void *p, size_t n)
{
    if (!p) return malloc(n);
    if (!n) { free(p); return NULL; }

    size_t old = (size_t)((common_header_t *)p - 1)->size;
    if (n <= old) return p;

    void *q = malloc(n);
    if (q) {
        memcpy(q, p, old);
        free(p);
    }
    return q;
}
