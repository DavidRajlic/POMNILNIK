#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

struct alok_info {
   size_t velikost_alokacije; // velikost alokacije uporabljena pri mmap
   size_t stevilo_odsekov;    // trenutno število odsekov
   void *prazen_odsek;        // kazalec na prazen odsek
   void *next;
};

struct odsek_info {
   size_t velikost_odseka; // velikost odseka
   void *stran;            // kazalec na začetek alocirane strani
};

int st_strani = 0;
int st_odsekov = 0;
void *zadnji_naslov = NULL;
void *zacetek_naslov = NULL;

struct  alok_info *first = NULL;
struct  alok_info *current = NULL;

void *mymalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    int velikost_strani = getpagesize();

    if (first == NULL || size > PAGE_SIZE) {
        size_t velikost_alokacije = size + sizeof(struct alok_info) + sizeof(struct odsek_info);

        size_t ostanek = velikost_alokacije % velikost_strani;
        if (ostanek > 0) {
            velikost_alokacije += velikost_strani - ostanek;
        }

        void *stran = mmap(NULL, velikost_alokacije, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (stran == MAP_FAILED) {
            return NULL;
        }
        st_strani++;
        st_odsekov++;

        struct alok_info *new_alok_info = (struct alok_info *)stran;
        new_alok_info->velikost_alokacije = velikost_alokacije;
        new_alok_info->stevilo_odsekov = st_odsekov;
        new_alok_info->prazen_odsek = stran + sizeof(struct odsek_info);
        zadnji_naslov = new_alok_info->prazen_odsek;
        current = new_alok_info;
        return new_alok_info->prazen_odsek;
    } else {
        struct odsek_info *odsek_info = zadnji_naslov;
        odsek_info->stran = zadnji_naslov + size + sizeof(struct odsek_info);
        zadnji_naslov = odsek_info->stran;
        return zadnji_naslov;
    }
}

void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    // Koda za sproščanje pomnilnika
    while (st_strani > 0) {
        if (munmap(current, current->velikost_alokacije) == -1) {
            perror("Error in munmap");
        } else {
            st_strani--;
        }
    }
    first = NULL;
}




// gcc main.c -o main.o -c
// gcc mymalloc.c -o mymalloc.o -c
// gcc -o main main.o mymalloc.o