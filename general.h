#ifndef GENERAL_H
#define GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <complex.h>

#define INIT_BSIZE 512
    
#define NUMERR 0.000001
#define EQN(x,n) (x > n - NUMERR && x < n + NUMERR)
#define EQN1(x) EQN(x,-1)
#define EQ0(x) EQN(x,0)
#define EQ1(x) EQN(x,1)

typedef struct buf_s buf_s;

struct buf_s
{
    size_t bsize;
    size_t size;
    char *buf;
};

extern buf_s *bufinit(void);
extern void bufaddc(buf_s *b, char c);
extern void bufaddstr(buf_s *b, char *str, size_t len);
extern void bufadddouble(buf_s *b, double val);
extern void bufaddcomplex(buf_s *b, complex double val);
extern void buf_trim(buf_s *b);

extern void *alloc(size_t size);
extern void *allocz(size_t size);
extern void *ralloc(void *ptr, size_t size);

extern short ndigits(int num);

#endif

#ifdef __cplusplus
}
#endif
