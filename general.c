#include "general.h"
#include <stdio.h>

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


short ndigits(int num)
{
    int c;
    
    if(!num)
        return 1;
    for(c = 0; num; c++)
        num /= 10;
    return c;
}
