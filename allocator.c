#include <stdio.h>
#include <sys/mman.h>
#include "allocator.h"
#include "freelist.h"

typedef struct {
    const char       *name;
    char             *base;
    size_t            size;
    common_header_t  *head;
    int               ready;
} zone_t;

#if ZONES
#define TINY_MAX   256
#define SMALL_MAX  (8 * 1024)
#define TINY_SZ    (MEM_SIZE / 10)
#define SMALL_SZ   (3 * (MEM_SIZE / 10))
#define MEDIUM_SZ  (MEM_SIZE - TINY_SZ - SMALL_SZ)
static zone_t zones[] = {
    { "TINY",   NULL, TINY_SZ,   NULL, 0 },
    { "SMALL",  NULL, SMALL_SZ,  NULL, 0 },
    { "MEDIUM", NULL, MEDIUM_SZ, NULL, 0 },
};
#else
static zone_t zones[] = { { "HEAP", NULL, MEM_SIZE, NULL, 0 } };
#endif
#define NZONES ((int)(sizeof zones / sizeof zones[0]))

static size_t scan_visited = 0, scan_calls = 0;

static void *get_mem_block(void *addr, size_t mem_size)
{
    void *p = mmap(addr, mem_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? NULL : p;
}

static int zone_init(zone_t *z)
{
    if (z->ready) return 1;
    z->base = get_mem_block(NULL, z->size);
    if (!z->base) return 0;
    fl_init(&z->head, z->base, z->size);
    z->ready = 1;
    return 1;
}

static int zone_for(size_t total)
{
#if ZONES
    if (total <= TINY_MAX)  return 0;
    if (total <= SMALL_MAX) return 1;
    return 2;
#else
    (void)total;
    return 0;
#endif
}

static zone_t *owner_of(const void *p)
{
    for (int z = 0; z < NZONES; z++) {
        const char *c = (const char *)p;
        if (zones[z].base && c >= zones[z].base && c < zones[z].base + zones[z].size)
            return &zones[z];
    }
    return NULL;
}

void *smalloc(size_t num_bytes)
{
    if (num_bytes == 0) return NULL;
    size_t n = ROUND_UP(num_bytes);
    scan_calls++;

    for (int z = zone_for(n + HDR_SIZE); z < NZONES; z++) {
        if (!zone_init(&zones[z])) return NULL;
        common_header_t *prev, *blk;
#if FIT
        blk = fl_find_best_fit(zones[z].head, n, &prev, &scan_visited);
#else
        blk = fl_find_first_fit(zones[z].head, n, &prev, &scan_visited);
#endif
        if (blk) {
            fl_take(&zones[z].head, prev, blk, n);
            return blk + 1;
        }
    }
    return NULL;
}

void sfree(void *mem_ptr)
{
    if (!mem_ptr) return;
    zone_t *z = owner_of(mem_ptr);
    if (!z) return;

    common_header_t *hdr = (common_header_t *)mem_ptr - 1;
#if MERGE
    common_header_t *prev;
    fl_insert_ordered(&z->head, hdr, &prev);
    fl_merge(prev, hdr);
#else
    fl_insert_head(&z->head, hdr);
#endif
}

void allocator_list_dump(void)
{
    for (int z = 0; z < NZONES; z++) {
        if (NZONES > 1) printf("%s%s: ", z ? "           " : "", zones[z].name);
        fl_dump(zones[z].head);
    }
}

size_t allocator_free_mem_size(void)
{
    size_t total = 0;
    for (int z = 0; z < NZONES; z++) total += fl_total_free(zones[z].head);
    return total;
}

size_t allocator_req_mem(size_t payload)
{
    return ROUND_UP(payload) + HDR_SIZE;
}

void allocator_stats(size_t *N, size_t *F, size_t *L)
{
    *N = *F = *L = 0;
    for (int z = 0; z < NZONES; z++) {
        size_t n, f, l;
        fl_stats(zones[z].head, &n, &f, &l);
        *N += n;
        *F += f;
        if (l > *L) *L = l;
    }
}

int allocator_num_zones(void) { return NZONES; }

const char *allocator_zone_name(int z) { return zones[z].name; }

void allocator_zone_stats(int z, size_t *N, size_t *F, size_t *L)
{
    fl_stats(zones[z].head, N, F, L);
}

void allocator_scan_stats(size_t *visited, size_t *calls)
{
    *visited = scan_visited;
    *calls   = scan_calls;
}
