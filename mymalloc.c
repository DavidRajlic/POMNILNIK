#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

struct alok_info
{
    size_t velikost_strani;
    size_t stevilo_odsekov;
    struct alok_info *nova_stran; // naslednji prost
    struct odsek_info *firstOdsek;
};

struct odsek_info
{
    size_t velikost_odseka;
    struct alok_info *alok_info;
    struct odsek_info *next;
};

struct alok_info *glava = NULL;

void *mymalloc(size_t size)
{

    if (size == 0)
    {
        return NULL;
    }

    // Prvo alociranje
    if (glava == NULL)
    {
        size_t velikost_alokacije = size + sizeof(struct odsek_info) + sizeof(struct alok_info);

        size_t velikost_strani = getpagesize(); // 4096 zlogov

        size_t ostanek = velikost_alokacije % velikost_strani;
        if (ostanek > 0)
        {
            velikost_alokacije += velikost_strani - ostanek;
        }
        struct alok_info *stran = mmap(NULL, velikost_alokacije, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (stran == MAP_FAILED) // Če je napaka pri alokaciji
        {
            return NULL;
        }
        // Nastavi vrednosti alokacije
        stran->velikost_strani = velikost_alokacije;
        stran->firstOdsek = (struct odsek_info *)(stran + 1);
        stran->firstOdsek->alok_info = stran;
        stran->firstOdsek->next = NULL;
        stran->firstOdsek->velikost_odseka = size;
        glava = (struct alok_info *)stran;
        stran->stevilo_odsekov = 1;
        return (char *)(glava) + sizeof(struct alok_info) + sizeof(struct odsek_info);
    }
    else
    {   //Če stran že obstaja
        struct alok_info *currentStran = glava;
        struct odsek_info *currentOdsek = currentStran->firstOdsek;
        while (currentOdsek->next != NULL) // gremo do zadnjega odseka
        {
            currentOdsek = currentOdsek->next;
        }
        size_t zapolnjeno = ((char *)currentOdsek + currentOdsek->velikost_odseka - (char *)currentStran); // izračun zapolnjnenga dela strani

        if (currentStran->velikost_strani - zapolnjeno > size) // ali je še dovolj prostora v tej strani
        {
            if (currentOdsek->next == NULL)
            {
                currentOdsek->next = (struct odsek_info *)((char *)(currentOdsek + 1) + size);
                currentOdsek->next->alok_info = glava;
                currentOdsek->next->velikost_odseka = size;
                currentOdsek->next->next = NULL;
                glava->stevilo_odsekov++;
                return (void *)(currentOdsek->next + 1);
            }
        }
        else // Če ni dovolj prostora bomo ustvarili novo stran
        {
            size_t velikost_alokacije = size + sizeof(struct odsek_info) + sizeof(struct alok_info);
            size_t velikost_strani = getpagesize();

            size_t ostanek = velikost_alokacije % velikost_strani;
            if (ostanek > 0)
            {
                velikost_alokacije += velikost_strani - ostanek;
            }

            struct alok_info *stran = mmap(NULL, velikost_alokacije, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            if (stran == MAP_FAILED)
            {
                return NULL;
            }
            stran->velikost_strani = velikost_alokacije;

            stran->firstOdsek = (struct odsek_info *)(stran + 1);

            stran->firstOdsek->alok_info = stran;
            stran->firstOdsek->next = NULL;
            stran->firstOdsek->velikost_odseka = size;
            stran->stevilo_odsekov = 1;
            currentStran->nova_stran = (struct alok_info *)stran;
            return (char *)(currentStran->nova_stran) + sizeof(struct alok_info) + sizeof(struct odsek_info);
        }
    }

    return NULL;
}

void myfree(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    struct odsek_info *odsek = ((struct odsek_info *)ptr) - 1;
    struct alok_info *stran = odsek->alok_info;
    if (stran == glava)
    {
        if (glava->nova_stran == NULL)
        {
            glava = NULL;
        }
        else
        {
            glava = glava->nova_stran;
        }
    }

    stran->stevilo_odsekov--;
    if (stran->stevilo_odsekov == 0)
    {
        if (munmap(stran, stran->velikost_strani) == -1)
        {
            perror("Error in munmap");
        }
    }
}