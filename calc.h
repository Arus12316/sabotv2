#ifndef __botparse__calc__
#define __botparse__calc__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct term_s term_s;
typedef struct cnode_s cnode_s;
typedef struct calcres_s calcres_s;

struct cnode_s {
    double coeff;
    double exp;
    char *ident;
    term_s *next;
};

struct calcres_s
{
    int status;
    char *val;
};
    
extern calcres_s eval(char *exp);

#ifdef __cplusplus
}
#endif

#endif /* defined(__botparse__calc__) */
