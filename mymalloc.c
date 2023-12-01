#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

struct alok_info {
    size_t velikost_alokacije; // velikost alokacije uporabljena pri mmap
    size_t stevilo_odsekov;    // trenutno število odsekov
    void *prazen_odsek;        // kazalec na prazen odsek
};

struct odsek_info {
    size_t velikost_odseka; // velikost odseka
    void *stran;            // kazalec na začetek alocirane strani
};

void *mymalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t velikost_alokacije = size + sizeof(struct odsek_info);
    int velikost_strani = getpagesize();
    size_t ostanek = velikost_alokacije % velikost_strani;
    if (ostanek > 0) {
        velikost_alokacije += velikost_strani - ostanek;
    }

    void *stran = mmap(NULL, velikost_alokacije, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stran == MAP_FAILED) {
        return NULL;
    }

    struct odsek_info *odsek_info = (struct odsek_info *)stran;
    odsek_info->velikost_odseka = velikost_alokacije - sizeof(struct odsek_info);
    odsek_info->stran = stran;

    return (void *)(odsek_info + 1); // Vrni naslov začetka podatkov v odseku
}

void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    struct odsek_info *odsek_info = ((struct odsek_info *)ptr) - 1;
    size_t heap_size = ((odsek_info->velikost_odseka + sizeof(struct odsek_info) + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

    if (munmap(odsek_info->stran, heap_size) == -1) {
        perror("Error in munmap");
    }
}


// gcc main.c -o main.o -c
// gcc mymalloc.c -o mymalloc.o -c
// gcc -o main main.o mymalloc.o