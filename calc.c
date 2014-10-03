#include "calc.h"
#include "general.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <stdarg.h>

#define TOK() (ti->curr)
#define NEXTTOK() (puts(ti->curr->next->lex), ti->curr = ti->curr->next)

#define TC(C) TCC(C)
#define TCC(C) #C

#define ERR(f,...) err(ti, f, __VA_ARGS__)

typedef struct tok_s tok_s;
typedef struct tokiter_s tokiter_s;
typedef struct opnode_s opnode_s;
typedef struct numnode_s numnode_s;
typedef struct idnode_s idnode_s;
typedef struct node_s node_s;

typedef enum {
    CALCTOK_MULOP,
    CALCTOK_ADDOP,
    CALCTOK_IDENT,
    CALCTOK_NUM,
    CALCTOK_EXPON,
    CALCTOK_OPENPAREN,
    CALCTOK_CLOSEPAREN,
    CALCTOK_EOF
}
ttype_e;

typedef enum {
    CALCATT_DEFAULT,
    CALCATT_MULT,
    CALCATT_DIV,
    CALCATT_MOD,
    CALCATT_ADD,
    CALCATT_SUB
}
ttatt_e;

typedef enum {
    NTYPE_OP,
    NTYPE_NUM,
    NTYPE_ID
}
ntype_e;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MULT,
    OP_DIV,
    OP_MOD,
    OP_EXP
}
op_e;

struct tok_s {
    char *lex;
    ttype_e type;
    ttatt_e att;
    tok_s *next;
};

struct tokiter_s
{
    tok_s *curr;
    bool issuccess;
};

struct opnode_s
{
    int weight;
    op_e val;
    union {
        struct {
            node_s *l, *r;
        };
        node_s *c;
    };
};

struct numnode_s
{
    double val;
};

struct idnode_s
{
    tok_s *t;
};

struct node_s {
    ntype_e type;
    union {
        opnode_s op;
        numnode_s num;
        idnode_s ident;
    };
    node_s *p;
};

static tok_s *lex(tokiter_s *ti, char *src);
static void mktok(tok_s **list, char *lex, ttype_e type, ttatt_e);
static void print_tok(tok_s *tok);
static void free_tok(tok_s *tok);

/*

 <start> -> <expression> | ε
 
 <expression> -> <term> <expression'>
 
 <expression'> -> addop <term> <expression'> | ε
 
 <term> -> <subterm> <term'>
 
 <term'> -> mulop <subterm> <term'> | ε
 
 <optident> -> ident | ε
 
 <subterm> -> <factor> <subterm'>
 
 <subterm'> -> expon <factor> <subterm'> | ε
 
 <factor> -> num | ident <func> | ( <expression> ) <optfactor> | addop <factor>
 
 <optfactor> -> <factor> | ε
 
 <numfollow> -> <factor> ~excluding num
 
 <func> -> (<expression>) <optfactor> | ε
 
 <paramlist> -> <expression> <paramlist'> | ε
 
 <paramlist'> -> , <expression>
 
 */

static calcres_s p_start(tokiter_s *ti);
static double p_expression(tokiter_s *ti);
static void p_expression_(tokiter_s *ti, double *accum);
static double p_term(tokiter_s *ti);
static void p_term_(tokiter_s *ti, double *accum);
static double p_subterm(tokiter_s *ti);
static double p_subterm_(tokiter_s *ti);
static node_s *p_factor(tokiter_s *ti);
static void p_optfactor(tokiter_s *ti, double *accum);

static node_s *node_s_(ntype_e type);

static void err(tokiter_s *ti, const char *f, ...);

calcres_s eval(char *exp)
{
    tok_s *list;
    tokiter_s ti;
    calcres_s res;

    char *bck = alloc(strlen(exp));
    strcpy(bck, exp);
    
    ti.issuccess = true;
    list = lex(&ti, bck);
    res = p_start(&ti);
    free_tok(list);
    free(bck);
    return res;
}

tok_s *lex(tokiter_s *ti, char *src)
{
    bool gotdec;
    char *bptr, bck;
    tok_s *head, *curr;
    
    curr = head = alloc(sizeof *head);
    
    while(*src) {
        switch(*src) {
            case ' ':
            case '\n':
            case '\t':
            case '\v':
                src++;
                break;
            case '*':
                mktok(&curr, "+", CALCTOK_MULOP, CALCATT_MULT);
                src++;
                break;
            case '/':
                mktok(&curr, "+", CALCTOK_MULOP, CALCATT_DIV);
                src++;
                break;
            case '|':
            case '+':
                mktok(&curr, "+", CALCTOK_ADDOP, CALCATT_ADD);
                src++;
                break;
            case '-':
                mktok(&curr, "-", CALCTOK_ADDOP, CALCATT_SUB);
                src++;
                break;
            case '^':
                mktok(&curr, "-", CALCTOK_EXPON, CALCATT_DEFAULT);
                src++;
                break;
            case '(':
                mktok(&curr, "(", CALCTOK_OPENPAREN, CALCATT_DEFAULT);
                src++;
                break;
            case ')':
                mktok(&curr, ")", CALCTOK_CLOSEPAREN, CALCATT_DEFAULT);
                src++;
                break;
            default:
                if(isalpha(*src) || *src == '-') {
                    bptr = src;
                    while(isalnum(*++src) || *src == '_');
                    bck = *src;
                    *src = '\0';
                    if(!strcasecmp(bptr, "plus")) {
                        mktok(&curr, "+", CALCTOK_ADDOP, CALCATT_ADD);
                    }
                    else if(!strcasecmp(bptr, "pi")) {
                        mktok(&curr, TC(M_PI), CALCTOK_NUM, CALCATT_DEFAULT);
                    }
                    else if(!strcmp(bptr, "e")) {
                        mktok(&curr, TC(M_E), CALCTOK_NUM, CALCATT_DEFAULT);
                    }
                    else if(!strcmp(bptr, "mod")) {
                        mktok(&curr, "mod", CALCTOK_MULOP, CALCATT_MOD);
                    }
                    else {
                        mktok(&curr, bptr, CALCTOK_IDENT, CALCATT_DEFAULT);
                    }
                    *src = bck;
                }
                else if(isdigit(*src) || *src == '.') {
                    gotdec = *src == '.';
                    bptr = src;
                    while(isdigit(*++src));
                    if(*src == '.') {
                        if(gotdec) {
                            fprintf(stderr, "Lexical Error: Improperly formed number\n");
                        }
                        else {
                            while(isdigit(*++src));
                        }
                    }
                    if(*src == 'e' || *src == 'E') {
                        src++;
                        gotdec = false;
                        if(*src == '+' || *src == '-')
                            src++;
                        while(true) {
                            if(isdigit(*src)) {
                                src++;
                            }
                            else if(*src == '.') {
                                src++;
                                if(gotdec) {
                                    ERR("Lexical Error: Improperly formed number in exponent at %c\n", *src);
                                    break;
                                }
                                gotdec = true;
                            }
                            else {
                                if(*(src - 1) == '.' && !isdigit((*src - 2))) {
                                    ERR("Lexical Error: Improperly formed number in exponent at %c\n", *(src - 1));
                                }
                                else if(*(src - 1) == '-' || *(src - 1) == '+') {
                                    ERR("Lexical Error: Improperly formed number in exponent at %c\n", *(src - 1));
                                }
                                break;
                            }
                        }
                    }
                    
                    bck = *src;
                    *src = '\0';
                    mktok(&curr, bptr, CALCTOK_NUM, CALCATT_DEFAULT);
                    *src = bck;
                }
                else {
                    ERR("Lexical Error: Unknown Symbol %c\n", *src);
                    src++;
                }
                break;
        }
    }
    mktok(&curr, "EOF", CALCTOK_EOF, CALCATT_DEFAULT);
    ti->curr = head->next;
    return head;
}

void mktok(tok_s **list, char *lex, ttype_e type, ttatt_e att)
{
    tok_s *nt;
    char *lexcpy;
    
    nt = alloc(sizeof *nt);
    lexcpy = alloc(strlen(lex));
    strcpy(lexcpy, lex);
    nt->lex = lexcpy;
    nt->type = type;
    nt->att = att;
    nt->next = NULL;
    (*list)->next = nt;
    *list = nt;
}

void print_tok(tok_s *tok)
{
    tok = tok->next;
    while(tok) {
        printf("%s %d %d\n", tok->lex, tok->type, tok->att);
        tok = tok->next;
    }
}

void free_tok(tok_s *tok)
{
    tok_s *bck;
    
    bck = tok;
    tok = tok->next;
    free(bck);
    while(tok) {
        bck = tok->next;
        free(tok->lex);
        free(tok);
        tok = bck;
    }
}

calcres_s p_start(tokiter_s *ti)
{
    double result;
    calcres_s cres;
    tok_s *t = TOK();
    
    if(t->type != CALCTOK_EOF) {
        result = p_expression(ti);
        t = TOK();
        if(t->type == CALCTOK_EOF) {
            if(ti->issuccess)
                printf("parse success!\nresult: %f\n", result);
        }
        else {
            ERR("Syntax Error: Expected EOF but got %s\n", t->lex);
        }
    }
    else {
        result = 0;
        ti->issuccess = false;
    }
    cres.status = !ti->issuccess;
    cres.val = result;
    return cres;
}

double p_expression(tokiter_s *ti)
{
    double accum;
    
    accum = p_term(ti);
    p_expression_(ti, &accum);
    return accum;
}

void p_expression_(tokiter_s *ti, double *accum)
{
    double term;
    tok_s *t = TOK(), *bck;

    if(t->type == CALCTOK_ADDOP) {
        bck = t;
        NEXTTOK();
        term = p_term(ti);
        if(bck->att == CALCATT_ADD) {
            *accum += term;
        }
        else {
            *accum -= term;
        }
        p_expression_(ti, accum);
    }
}

double p_term(tokiter_s *ti)
{
    double accum;
    
    accum = p_subterm(ti);
    p_term_(ti, &accum);
    return accum;
}

void p_term_(tokiter_s *ti, double *accum)
{
    double subterm, exp;
    long long iterm;
    tok_s *t = TOK(), *bck;
    
    if(t->type == CALCTOK_MULOP) {
        bck = t;
        NEXTTOK();
        subterm = p_subterm(ti);
        if(bck->att == CALCATT_MULT) {
            *accum *= subterm;
        }
        else if(bck->att == CALCATT_MOD) {
            iterm = (long long)*accum;
            iterm %= (long long)subterm;
            *accum = iterm;
        }
        else {
            *accum /= subterm;
        }
        p_term_(ti, accum);
    }
}

double p_subterm(tokiter_s *ti)
{
    double accum, subterm_;
    
    accum = p_factor(ti);
    subterm_ = p_subterm_(ti);
    return pow(accum, subterm_);
}

double p_subterm_(tokiter_s *ti)
{
    double factor, subterm_;
    tok_s *t = TOK();

    if(t->type == CALCTOK_EXPON) {
        NEXTTOK();
        factor = p_factor(ti);
        subterm_ = p_subterm_(ti);
        return pow(factor, subterm_);
    }
    return 1;
}

node_s *p_factor(tokiter_s *ti)
{
    int mult;
    double val, valf;
    tok_s *t = TOK(), *bck;
    node_s *res, *p, *l, *r;
    
    switch(t->type) {
        case CALCTOK_NUM:
            bck = t;
            t = NEXTTOK();
            val = atof(bck->lex);
            if(t->type == CALCTOK_IDENT || t->type == CALCTOK_OPENPAREN) {
                r = p_factor(ti);
                if(r->type == NTYPE_NUM) {
                    r->num.val *= val;
                    res = r;
                }
                else if(r->type == NTYPE_ID) {
                    p = node_s_(NTYPE_OP);
                    p->op.val = OP_MULT;
                    
                    if(val == 0.0) {
                        
                    }
                    else {
                        
                    }
                    l = node_s_(NTYPE_NUM);
                    l->num.val = val;

                    p->op.l = n;
                    p->op.r = nn;
                    n->p = p;
                    nn->p = p;
                }
            }
            else {
                n = node_s_(NTYPE_NUM);
                n->num.val = val;
            }
            return n;
        case CALCTOK_IDENT:
            bck = t;
            NEXTTOK();
            t = TOK();
            if(t->type == CALCTOK_OPENPAREN) {
                NEXTTOK();
                val = p_expression(ti);
                t = TOK();
                if(t->type != CALCTOK_CLOSEPAREN) {
                    ERR("Syntax Error: Expected ) but got %s\n", t->lex);
                }
                NEXTTOK();
                if(!strcmp(bck->lex, "sin")) {
                    val = sin(val);
                }
                else if(!strcmp(bck->lex, "cos")) {
                    val = cos(val);
                }
                else if(!strcmp(bck->lex, "tan")) {
                    val = tan(val);
                }
                else if(!strcmp(bck->lex, "arcsin")) {
                    val = asin(val);
                }
                else if(!strcmp(bck->lex, "arccos")) {
                    val = acos(val);
                }
                else if(!strcmp(bck->lex, "arctan")) {
                    val = atan(val);
                }
                else if(!strcmp(bck->lex, "log")) {
                    val = log10(val);
                }
                else if(!strcmp(bck->lex, "ln")) {
                    val = log(val);
                }
                else if(!strcmp(bck->lex, "lg")) {
                    val = log2(val);
                }
                else if(!strcmp(bck->lex, "sqrt")) {
                    val = sqrt(val);
                }
                p_optfactor(ti, &val);
            }
            else {
                val = 0;
            }
            return val;
        case CALCTOK_OPENPAREN:
            NEXTTOK();
            val = p_expression(ti);
            t = TOK();
            if(t->type == CALCTOK_CLOSEPAREN) {
                NEXTTOK();
                p_optfactor(ti, &val);
            }
            else {
                ERR("Syntax Error: Expected ) but got %s\n", t->lex);
            }
            return val;
        case CALCTOK_ADDOP:
            if(t->att == CALCATT_SUB)
                mult = -1;
            else
                mult = 1;
            NEXTTOK();
            return mult*p_factor(ti);
            break;
        default:
            ERR("Syntax Error: Expected number, identifier, (, +, or -, but got %s\n", t->lex);
            return 0;
    }
}

void p_optfactor(tokiter_s *ti, double *accum)
{
    double val;
    tok_s *t = TOK();
    
    switch (t->type) {
        case CALCTOK_NUM:
        case CALCTOK_IDENT:
        case CALCTOK_OPENPAREN:
            val = p_factor(ti)->num.val;
            *accum *= val;
            break;
        default:
            break;
    }
}

node_s *node_s_(ntype_e type)
{
    node_s *n;
    
    n = allocz(sizeof *n);
    n->type = type;
    return n;
}

void err(tokiter_s *ti, const char *f, ...)
{
    va_list args;
    va_start(args, f);
    vfprintf(stderr, f, args);
    va_end(args);

    ti->issuccess = false;
}
