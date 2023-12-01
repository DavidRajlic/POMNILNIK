#define _GNU_SOURCE
#include "mymalloc.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <bits/mman-linux.h>

#define PAGE_SIZE 4096

void *mymalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t total_size = size + sizeof(size_t);

    size_t heap_size = ((total_size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    void *heap_ptr = mmap(NULL, heap_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (heap_ptr == MAP_FAILED) {
        return NULL;
    }

    *((size_t *)heap_ptr) = heap_size - sizeof(size_t);
    void *allocated = heap_ptr + sizeof(size_t);

    return allocated;
}

void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    void *block_start = ptr - sizeof(size_t);
    size_t size = *((size_t *)block_start);

    size_t heap_size = ((size + sizeof(size_t) + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    if (munmap(block_start, heap_size) == -1) {
        perror("Error in munmap");
    }
}

// gcc main.c -o main.o -c
// gcc mymalloc.c -o mymalloc.o -c
// gcc -o main main.o mymalloc.o