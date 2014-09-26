#ifndef __botparse__calc__
#define __botparse__calc__

#include <stdio.h>

typedef struct term_s term_s;

struct term_s {
    double coeff;
    double exp;
    char *ident;
    term_s *next;
};

extern void eval(char *exp);


#endif /* defined(__botparse__calc__) */
