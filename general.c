#include "general.h"

void *alloc(size_t n)
{
    void *p = malloc(n);

    if(!p) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    return p;
}

void *allocz(size_t n)
{
    void *p = calloc(1, n);

    if(!p) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    return p;
}

void *ralloc(void *ptr, size_t n)
{
    void *p = realloc(ptr, n);
    if(!p) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }

    return p;
}
