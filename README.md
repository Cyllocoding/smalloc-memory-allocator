# smalloc — a malloc/free replacement over mmap arenas

A from-scratch dynamic memory allocator in C.

- Drop-in `malloc`/`free` replacement over `mmap`-reserved arenas, using an intrusive free list with in-band metadata headers, block splitting, address-ordered insertion, and adjacent-block coalescing.
- First-fit vs best-fit placement and the effect of coalescing benchmarked under a 50,000-request stress harness, instrumenting external fragmentation, heap utilisation at failure, peak free-list length, and average scan length per allocation.
- Extended to segregated size-class arenas (TINY / SMALL / MEDIUM, each `mmap`-backed with its own free list and escalation on exhaustion), routing by request size and resolving zone ownership on free by pointer range.

## Layout

| File | Role |
|---|---|
| `freelist.h/.c` | Header struct and pure list operations (init, find, split/take, insert, merge, stats). No globals. |
| `allocator.h/.c` | `smalloc`/`sfree`, arena setup via `mmap`, zone routing, utility and stats functions. |
| `main.c` | Small functional test (1 KB heap). |
| `allocation_stress_test.c` | 50,000-request stress harness (10 MB heap). |
| `malloc_shim.c` | Exports `malloc`/`free`/`calloc`/`realloc` for `LD_PRELOAD`. |
| `Makefile` | Build matrix. |

## Design

**Block layout.** Every block, free or allocated, starts with a 16-byte header:

```
+----------------+-----------------------------+
| size | next    |        payload (size B)     |
+----------------+-----------------------------+
^ header         ^ pointer returned to the user
```

`next` links free blocks into the free list and is `NULL` while the block is allocated. The free list lives inside the free blocks themselves (intrusive), so it costs no extra memory.

**Alignment.** Every request is rounded up to a multiple of 16, so all headers and user pointers are 16-byte aligned.

**smalloc.** Round the size, pick the zone, search its free list (first-fit or best-fit), split the chosen block if the leftover can hold a header plus 16 bytes, otherwise hand out the whole block. If no block fits, escalate to the next larger zone.

**sfree.** Step back one header, find the owning zone by pointer range, then either push onto the list head (`MERGE=0`) or insert in address order and coalesce with touching neighbours (`MERGE=1`).

**Zones.** With `ZONES=1` the 10 MB heap is split into TINY (1 MB, requests ≤ 256 B incl. header), SMALL (3 MB, ≤ 8 KB) and MEDIUM (6 MB, the rest). Each zone is a separate `mmap` with its own free list; the same `freelist.c` code serves all three.

## Build and run

Compile-time switches: `FIT` (0 first-fit, 1 best-fit), `MERGE` (0/1), `ZONES` (0/1).

```sh
make test  FIT=0 MERGE=0 && ./test            # Version 1 on a 1 KB heap
make test  FIT=1 MERGE=1 && ./test            # Version 2
make stress FIT=1 MERGE=1 ZONES=1 && ./stress # one stress run
make cases                                    # full matrix, outputs in results/
make shim && LD_PRELOAD=./libsmalloc.so ls    # drop-in use (Linux)
```

On macOS use `DYLD_INSERT_LIBRARIES=./libsmalloc.so DYLD_FORCE_FLAT_NAMESPACE=1 ./your_program` and build with `-dynamiclib`; system binaries are protected by SIP, so test with a locally built program.

## Results

Stress harness: 10 MB heap, 50,000 requests, sizes up to 32 KB, 512 live blocks, one random free every 128 requests.

### Single heap: fit policy × coalescing

| Metric | Case 1: first-fit, no merge | Case 2: best-fit, no merge | Case 3a: first-fit + merge | Case 3b: best-fit + merge |
|---|---|---|---|---|
| Successful requests (%) | | | | |
| Requests before first failure | | | | |
| Utilisation at failure | | | | |
| BTF (× heap) | | | | |
| Peak free-list length | | | | |
| External fragmentation (max) | | | | |
| Avg nodes scanned per smalloc | | | | |

### Size-class arenas (best-fit + merge)

| Metric | Single heap | TINY / SMALL / MEDIUM |
|---|---|---|
| Successful requests (%) | | |
| Peak free-list length | | |
| External fragmentation (overall, max) | | |
| Per-zone fragmentation (final) | — | TINY: SMALL: MEDIUM: |
| Avg nodes scanned per smalloc | | |

### Observations

- _Fragmentation vs scan cost:_
- _Effect of coalescing:_
- _Size-class arenas vs single heap:_

## Notes

- The harness defines `MAX_REQ_SIZE` as `32 * 1024` without parentheses, so `rand() % MAX_REQ_SIZE` evaluates as `(rand() % 32) * 1024`. Request sizes are therefore `1, 1025, 2049, …, 31745`. This is kept as-is for comparability with the course's reference output, but it means almost every request lands in the MEDIUM zone, which strongly affects the zone results.
- `int size` in the header follows the course brief; it caps a single block at 2 GB.
- The shim serialises all calls with one mutex and is capped at `MEM_SIZE`; it is a demonstration, not a production allocator.
