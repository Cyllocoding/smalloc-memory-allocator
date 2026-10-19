#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

/* Build-time configuration (override with -D flags, see Makefile) */
#ifndef MEM_SIZE
#define MEM_SIZE (10 * 1024 * 1024)
#endif
#ifndef FIT
#define FIT 0        /* 0 = first-fit, 1 = best-fit */
#endif
#ifndef MERGE
#define MERGE 0      /* 1 = address-ordered insert + coalescing */
#endif
#ifndef ZONES
#define ZONES 0      /* 1 = TINY/SMALL/MEDIUM size-class arenas */
#endif

void *smalloc(size_t num_bytes);
void  sfree(void *mem_ptr);

void   allocator_list_dump(void);
size_t allocator_free_mem_size(void);
size_t allocator_req_mem(size_t payload);
void   allocator_stats(size_t *N, size_t *F, size_t *L);

int         allocator_num_zones(void);
const char *allocator_zone_name(int z);
void        allocator_zone_stats(int z, size_t *N, size_t *F, size_t *L);
void        allocator_scan_stats(size_t *visited, size_t *calls);

#endif
