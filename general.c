#include "general.h"
#include <stdio.h>
#include <string.h>

buf_s *bufinit(void)
{
    buf_s *b;
    
    b = alloc(sizeof *b);
    b->bsize = INIT_BSIZE;
    b->size = 0;
    b->buf = alloc(INIT_BSIZE);
    return b;
}

void bufaddc(buf_s *b, char c)
{
    b->size++;
    
    if(b->size > b->bsize) {
        b->bsize *= 2;
        b->buf = ralloc(b->buf, b->size);
    }
    b->buf[b->size - 1] = c;
}

void bufaddstr(buf_s *b, char *str, size_t len)
{
    b->size += len;

    if(b->size > b->bsize) {
        do {
            b->bsize *= 2;
        }
        while(b->size > b->bsize);
        b->buf = ralloc(b->buf, b->bsize);
    }
    strcpy(&b->buf[b->size - len], str);
}

void bufadddouble(buf_s *b, double val)
{
    int n;
    char buf[512];
    
    n = sprintf(buf, "%f", val);
    while(buf[--n] == '0')
        buf[n] = '\0';
    if(buf[n] == '.')
        buf[n] = '\0';
    bufaddstr(b, buf, strlen(buf));
}


void buf_trim(buf_s *b)
{
    b->bsize = b->size;
    b->buf = ralloc(b->buf, b->bsize);
}

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
