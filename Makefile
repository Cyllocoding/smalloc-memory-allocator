CC      = gcc
CFLAGS  = -Wall -Wextra -O2
FIT    ?= 0
MERGE  ?= 0
ZONES  ?= 0
DEFS    = -DFIT=$(FIT) -DMERGE=$(MERGE) -DZONES=$(ZONES)
CORE    = allocator.c freelist.c

.PHONY: all test stress cases shim clean

all: test stress

test: main.c $(CORE) allocator.h freelist.h
	$(CC) $(CFLAGS) $(DEFS) -DMEM_SIZE=1024 -o test main.c $(CORE)

stress: allocation_stress_test.c $(CORE) allocator.h freelist.h
	$(CC) $(CFLAGS) $(DEFS) -o stress allocation_stress_test.c $(CORE)

# Runs the full benchmark matrix and stores outputs under results/
cases:
	mkdir -p results
	$(MAKE) -B stress FIT=0 MERGE=0 ZONES=0 && ./stress > results/case1_firstfit_nomerge.txt
	$(MAKE) -B stress FIT=1 MERGE=0 ZONES=0 && ./stress > results/case2_bestfit_nomerge.txt
	$(MAKE) -B stress FIT=0 MERGE=1 ZONES=0 && ./stress > results/case3a_firstfit_merge.txt
	$(MAKE) -B stress FIT=1 MERGE=1 ZONES=0 && ./stress > results/case3b_bestfit_merge.txt
	$(MAKE) -B stress FIT=1 MERGE=1 ZONES=1 && ./stress > results/zones_bestfit_merge.txt
	@echo "Results written to results/"

shim: malloc_shim.c $(CORE)
	$(CC) $(CFLAGS) -fPIC -shared -DFIT=1 -DMERGE=1 -DZONES=1 -o libsmalloc.so malloc_shim.c $(CORE) -lpthread

clean:
	rm -f test stress libsmalloc.so
