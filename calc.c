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
#define NEXTTOK() (ti->curr = ti->curr->next)

#define TC(C) TCC(C)
#define TCC(C) #C

#define ERR(f,...) err(ti, f, __VA_ARGS__)

#define MAXLEXLEN 3

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
    CALCTOK_FACTORIAL,
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
    OP_EXP,
    OP_SIN,
    OP_COS,
    OP_TAN,
    OP_ARCSIN,
    OP_ARCCOS,
    OP_ARCTAN,
    OP_SINH,
    OP_COSH,
    OP_TANH,
    OP_LOG,
    OP_LN,
    OP_LG,
    OP_SQRT,
    OP_NEGATE,
    OP_FACTORIAL
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
    bool paren;
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
    complex double val;
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

static tok_s errtok = {
    .lex = "err",
    .type = CALCTOK_IDENT,
    .att = CALCATT_DEFAULT,
    .next = NULL
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
static node_s *p_expression(tokiter_s *ti);
static void p_expression_(tokiter_s *ti, node_s **acc);
static node_s *p_term(tokiter_s *ti);
static void p_term_(tokiter_s *ti, node_s **acc);
static node_s *p_subterm(tokiter_s *ti);
static node_s *p_subterm_(tokiter_s *ti);
static node_s *p_factor(tokiter_s *ti);
static void p_optfactor(tokiter_s *ti, node_s **accum);

static inline node_s *evalfunc(tok_s *t, node_s *arg);
static bool treeeq(node_s *r1, node_s *r2);
static bool isbinop(op_e op);

static node_s **add(node_s *n1, node_s *n2, ttatt_e att);

//static node_s **mult(node_s *n1, node_s *n2, ttatt_e att);


static node_s *node_s_(ntype_e type);

static void err(tokiter_s *ti, const char *f, ...);

static char *tostring(node_s *root);
static void tostring_(node_s *root, buf_s *buf);

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
            /*case '!':
                mktok(&curr, ")", CALCTOK_FACTORIAL, CALCATT_DEFAULT);
                src++;
                break;*/
            default:
                if(isalpha(*src) || *src == '-') {
                    bptr = src;
                    while(isalnum(*++src) || *src == '_');
                    bck = *src;
                    *src = '\0';
                    if(!strcasecmp(bptr, "plus")) {
                        mktok(&curr, "+", CALCTOK_ADDOP, CALCATT_ADD);
                    }
                    else if(!strcasecmp(bptr, "minus")) {
                        mktok(&curr, "-", CALCTOK_ADDOP, CALCATT_SUB);
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
                            ERR("Lexical Error: Improperly formed number at %c\n", '.');
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
    node_s *result;
    calcres_s cres;
    tok_s *t = TOK();
    
    if(t->type != CALCTOK_EOF) {
        result = p_expression(ti);
        t = TOK();
        if(t->type == CALCTOK_EOF) {
            if(ti->issuccess)
                printf("parse success!\nresult: %p\n", result);
        }
        else {
            ERR("Syntax Error: Expected EOF but got %s\n", t->lex);
        }
    }
    else {
        result = NULL;
        ti->issuccess = false;
    }
    cres.status = !ti->issuccess;
    cres.val = tostring(result);
    return cres;
}

node_s *p_expression(tokiter_s *ti)
{
    node_s *acc;
    
    acc = p_term(ti);
    p_expression_(ti, &acc);
    if(acc->type == NTYPE_OP) {
        acc->op.paren = true;
    }
    return acc;
}

void p_expression_(tokiter_s *ti, node_s **acc)
{
    node_s *term, *ac, *p, **branch;
    tok_s *t = TOK(), *bck;
    
    if(t->type == CALCTOK_ADDOP) {
        bck = t;
        ac = *acc;
        NEXTTOK();
        term = p_term(ti);
        if(term->type == NTYPE_NUM && ac->type == NTYPE_NUM) {
            if(bck->att == CALCATT_ADD) {
                ac->num.val += term->num.val;
            }
            else {
                ac->num.val -= term->num.val;
            }
            free(term);
            branch = acc;
        }
        else {
            p = node_s_(NTYPE_OP);
            if(bck->att == CALCATT_ADD) {
                p->op.val = OP_ADD;
            }
            else {
                p->op.val = OP_SUB;
            }

            if(treeeq(term, ac)) {
                puts("Equal!");
            }

            p->op.l = ac;
            p->op.r = term;
            ac->p = p;
            term->p = p;
            *acc = p;
            branch = &p->op.r;
        }
        p_expression_(ti, branch);
    }
}

node_s *p_term(tokiter_s *ti)
{
    node_s *acc;
    
    acc = p_subterm(ti);
    p_term_(ti, &acc);
    return acc;
}

void p_term_(tokiter_s *ti, node_s **acc)
{
    node_s *sub, *ac, *p, **branch, *iter, *tmp;
    long long iterm;
    tok_s *t = TOK(), *bck;
    
    if(t->type == CALCTOK_MULOP) {
        bck = t;
        ac = *acc;
        NEXTTOK();
        sub = p_subterm(ti);
        if(sub->type == NTYPE_NUM && ac->type == NTYPE_NUM) {
            if(bck->att == CALCATT_MULT) {
                ac->num.val *= sub->num.val;
            }
            else if(bck->att == CALCATT_DIV) {
                ac->num.val /= sub->num.val;
            }
            else {
                iterm = (long long)ac->num.val;
                iterm %= (long long)sub->num.val;
                ac->num.val = iterm;
            }
            free(sub);
            branch = acc;
        }
        else {
            for(iter = ac; iter; iter = iter->p) {
                if(iter->type == NTYPE_OP) {
                    tmp = iter->op.l;
                    if(sub->type == NTYPE_NUM && tmp->type == NTYPE_NUM) {
                        if(iter->op.val == OP_MULT) {
                            tmp->num.val *= sub->num.val;
                        }
                        else if(iter->op.val == OP_DIV) {
                            tmp->num.val /= sub->num.val;
                        }
                        else {
                            iterm = (long long)tmp->num.val;
                            iterm %= (long long)sub->num.val;
                            tmp->num.val = iterm;
                        }
                        free(sub);
                        branch = acc;
                        break;
                    }
                }
            }
            if(iter == NULL) {
                p = node_s_(NTYPE_OP);
                if(bck->att == CALCATT_MULT) {
                    p->op.val = OP_MULT;
                }
                else if(bck->att == CALCATT_DIV) {
                    p->op.val = OP_DIV;
                }
                else {
                    p->op.val = OP_MOD;
                }
                p->op.l = ac;
                p->op.r = sub;
                p->p = ac->p;
                ac->p = p;
                sub->p = p;
                *acc = p;
                branch = &p->op.r;
            }
        }
        //branch = mult(*acc, sub, bck->att);
        p_term_(ti, branch);
    }
}

node_s *p_subterm(tokiter_s *ti)
{
    node_s *fac, *sub, *p;
    
    fac = p_factor(ti);
    sub = p_subterm_(ti);
    if(sub) {
        if(fac->type == NTYPE_NUM && sub->type == NTYPE_NUM) {
            fac->num.val = cpow(fac->num.val, sub->num.val);
            free(sub);
            return fac;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_EXP;
            p->op.l = fac;
            p->op.r = sub;
            fac->p = p;
            sub->p = p;
            return p;
        }
    }
    else {
        return fac;
    }
}

node_s *p_subterm_(tokiter_s *ti)
{
    node_s *fac, *sub, *p;
    tok_s *t = TOK();

    if(t->type == CALCTOK_EXPON) {
        NEXTTOK();
        fac = p_factor(ti);
        sub = p_subterm_(ti);
        if(sub) {
            if(fac->type == NTYPE_NUM && sub->type == NTYPE_NUM) {
                fac->num.val = cpow(fac->num.val, sub->num.val);
                free(sub);
                return fac;
            }
            else {
                p = node_s_(NTYPE_OP);
                p->op.val = OP_EXP;
                p->op.l = fac;
                p->op.r = sub;
                fac->p = p;
                sub->p = p;
                return p;
            }
        }
        return fac;
    }
    return NULL;
}

node_s *p_factor(tokiter_s *ti)
{
    int mult;
    complex double val;
    tok_s *t = TOK(), *bck;
    node_s *res, *p, *l, *r, *c;
    
    switch(t->type) {
        case CALCTOK_NUM:
            bck = t;
            t = NEXTTOK();
            val = strtod(bck->lex, NULL);
            if(t->type == CALCTOK_IDENT || t->type == CALCTOK_OPENPAREN) {
                r = p_factor(ti);
                if(val == 0.0) {
                    res = node_s_(NTYPE_NUM);
                    res->num.val = val;
                }
                else {
                    if(r->type == NTYPE_NUM) {
                        r->num.val *= val;
                        res = r;
                    }
                    else {
                        p = node_s_(NTYPE_OP);
                        p->op.val = OP_MULT;
                        
                        l = node_s_(NTYPE_NUM);
                        l->num.val = val;
                        
                        p->op.l = l;
                        l->p = p;
                        p->op.r = r;
                        r->p = p;
                        res = p;
                    }
                }
            }
            else {
                res = node_s_(NTYPE_NUM);
                res->num.val = val;
            }
            return res;
        case CALCTOK_IDENT:
            bck = t;
            t = NEXTTOK();
            if(t->type == CALCTOK_OPENPAREN) {
                NEXTTOK();
                c = p_expression(ti);
                t = TOK();
                if(t->type != CALCTOK_CLOSEPAREN) {
                    ERR("Syntax Error: Expected ) but got %s\n", t->lex);
                }
                NEXTTOK();
                res = evalfunc(bck, c);
                if(!res) {
                    res = node_s_(NTYPE_ID);
                    res->ident.t = &errtok;
                    ERR("Too long identifier: %s\n", bck->lex);
                }
                p_optfactor(ti, &res);
            }
            else {
                if(strcmp(bck->lex, "i")) {
                    if(strlen(bck->lex) <= 3) {
                        res = node_s_(NTYPE_ID);
                        res->ident.t = bck;
                    }
                    else {
                        res = node_s_(NTYPE_ID);
                        res->ident.t = &errtok;
                        ERR("Too long identifier: %s\n", bck->lex);
                    }
                }
                else {
                    res = node_s_(NTYPE_NUM);
                    res->num.val = csqrt(-1);
                }
            }
            return res;
        case CALCTOK_OPENPAREN:
            NEXTTOK();
            res = p_expression(ti);
            t = TOK();
            if(t->type == CALCTOK_CLOSEPAREN) {
                NEXTTOK();
                p_optfactor(ti, &res);
            }
            else {
                ERR("Syntax Error: Expected ) but got %s\n", t->lex);
            }
            return res;
        case CALCTOK_ADDOP:
            if(t->att == CALCATT_SUB)
                mult = -1;
            else
                mult = 1;
            NEXTTOK();
            c = p_factor(ti);
            if(mult == -1) {
                if(c->type == NTYPE_NUM) {
                    c->num.val = -1*c->num.val;
                    res = c;
                }
                else {
                    p = node_s_(NTYPE_OP);
                    p->op.val = OP_NEGATE;
                    p->op.c = c;
                    c->p = p;
                    res = p;
                }
            }
            else {
                res = c;
            }
            return res;
        default:
            ERR("Syntax Error: Expected number, identifier, (, +, or -, but got %s\n", t->lex);
            p = node_s_(NTYPE_ID);
            p->ident.t = &errtok;
            return p;
    }
}

void p_optfactor(tokiter_s *ti, node_s **accum)
{
    tok_s *t = TOK();
    node_s *factor, *p, *acc;
    
    switch(t->type) {
        case CALCTOK_NUM:
        case CALCTOK_IDENT:
        case CALCTOK_OPENPAREN:
            factor = p_factor(ti);
            acc = *accum;
            if(acc->type == NTYPE_NUM && factor->type == NTYPE_NUM) {
                acc->num.val *= factor->num.val;
                free(factor);
            }
            else {
                p = node_s_(NTYPE_OP);
                p->op.val = OP_MULT;
                p->op.l = acc;
                p->op.r = factor;
                acc->p = p;
                factor->p = p;
                *accum = p;
            }
            break;
        default:
            break;
    }
}

inline node_s *evalfunc(tok_s *t, node_s *c)
{
    node_s *p, *res;
    
    if(!strcmp(t->lex, "sin")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = csin(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_SIN;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "cos")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = ccos(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_COS;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "tan")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = ctan(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_TAN;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "arcsin")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = casin(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_ARCSIN;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "arccos")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = cacos(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_ARCCOS;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "arctan")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = catan(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_ARCTAN;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "sinh")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = csinh(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_SINH;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "cosh")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = ccosh(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_COSH;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "tanh")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = ctanh(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_TANH;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "log")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = clog(c->num.val)/clog(10.0);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_LOG;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "ln")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = clog(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_LN;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "lg")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = clog(c->num.val)/clog(2.0);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_LG;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "sqrt")) {
        if(c->type == NTYPE_NUM) {
            c->num.val = csqrt(c->num.val);
            res = c;
        }
        else {
            p = node_s_(NTYPE_OP);
            p->op.val = OP_SQRT;
            p->op.c = c;
            c->p = p;
            res = p;
        }
    }
    else if(!strcmp(t->lex, "i")) {
        p = node_s_(NTYPE_OP);
        p->op.val = OP_MULT;
        p->op.l = node_s_(NTYPE_NUM);
        p->op.l->num.val = csqrt(-1);
        p->op.l->p = p;
        p->op.r = c;
        p->op.r->p = p;
        res = p;
    }
    else if(strlen(t->lex) > 3) {
        res = NULL;
    }
    else {
        p = node_s_(NTYPE_OP);
        p->op.val = OP_MULT;
        p->op.l = node_s_(NTYPE_ID);
        p->op.l->ident.t = t;
        p->op.l->p = p;
        p->op.r = c;
        p->op.r->p = p;
        res = p;
    }
    return res;
}

bool treeeq(node_s *r1, node_s *r2)
{
    if(r1->type != r2->type) {
        return false;
    }
    else {
        if(r1->type == NTYPE_OP) {
            if(r1->op.val != r2->op.val) {
                return false;
            }
            else {
                if(isbinop(r1->op.val)) {
                    return treeeq(r1->op.l, r2->op.l) && treeeq(r1->op.r, r2->op.r);
                }
                else {
                    return treeeq(r1->op.c, r2->op.c);
                }
            }
        }
        else if(r1->type == NTYPE_ID) {
            if(strcmp(r1->ident.t->lex, r2->ident.t->lex)) {
                return false;
            }
            else {
                return true;
            }
        }
        else {
            if(r1->num.val != r2->num.val) {
                return false;
            }
            else {
                return true;
            }
        }
    }
}

bool isbinop(op_e op)
{
    switch(op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MULT:
        case OP_DIV:
        case OP_MOD:
            return true;
        default:
            return false;
    }
}

node_s **add(node_s *n1, node_s *n2, ttatt_e att)
{
    return NULL;
}

/*
node_s **mult(node_s **acc, node_s *n2, ttatt_e att)
{
    long long iterm;
    node_s *i, *l, *p, *n1 = *acc;
    
    if(n1->type == NTYPE_NUM && n2->type == NTYPE_NUM) {
        switch(att) {
            case CALCATT_MULT:
                n1->num.val *= n2->num.val;
                break;
            case CALCATT_DIV:
                n1->num.val /= n2->num.val;
                break;
            case CALCATT_MOD:
                iterm = (long long)n1->num.val;
                iterm %= (long long)n2->num.val;
                n1->num.val = iterm;
                break;
            default:
                break;
        }
        free(n2);
        return acc;
    }
    else {
        for(i = n1; i; i = i->p) {
            if(i->type == NTYPE_OP) {
                l = i->op.l;
                if(treeeq(l, n2)) {
                    
                }
                else if(l->type == NTYPE_OP && l->op.val == OP_EXP) {
                    l = l->op.l;
                    if(treeeq(l, n2)) {
                        printf("yay");
                    }
                }
            }
        }
    }
}*/


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

char *tostring(node_s *root)
{
    buf_s buf;
    
    if(root) {
        buf.size = 0;
        buf.bsize = INIT_BSIZE;
        buf.buf = alloc(INIT_BSIZE);
        tostring_(root, &buf);
        bufaddc(&buf, '\0');
        return buf.buf;
    }
    return NULL;
}


void tostring_(node_s *root, buf_s *buf)
{
    if(root->type == NTYPE_NUM) {
        bufaddcomplex(buf, root->num.val);
    }
    else if(root->type == NTYPE_ID) {
        bufaddstr(buf, root->ident.t->lex, strlen(root->ident.t->lex));
    }
    else {
        switch(root->op.val) {
            case OP_ADD:
                if(root->op.l->type == NTYPE_OP && root->op.l->op.paren) {
                    bufaddc(buf, '(');
                    tostring_(root->op.l, buf);
                    bufaddc(buf, ')');
                }
                else {
                    tostring_(root->op.l, buf);
                }
                bufaddstr(buf, " plus ", sizeof(" plus ") - 1);
                if(root->op.r->type == NTYPE_OP && root->op.r->op.paren) {
                    bufaddc(buf, '(');
                    tostring_(root->op.r, buf);
                    bufaddc(buf, ')');
                }
                else {
                    tostring_(root->op.r, buf);
                }
                break;
            case OP_SUB:
                if(root->op.l->type == NTYPE_OP && root->op.l->op.paren) {
                    bufaddc(buf, '(');
                    tostring_(root->op.l, buf);
                    bufaddc(buf, ')');
                }
                else {
                    tostring_(root->op.l, buf);
                }
                bufaddstr(buf, " - ", sizeof(" - ") - 1);
                if(root->op.r->type == NTYPE_OP && root->op.r->op.paren) {
                    bufaddc(buf, '(');
                    tostring_(root->op.r, buf);
                    bufaddc(buf, ')');
                }
                else {
                    tostring_(root->op.r, buf);
                }
                break;
            case OP_MULT:
                if(root->op.paren && root->op.l->op.paren) {
                    bufaddc(buf, '(');
                    tostring_(root->op.l, buf);
                    bufaddc(buf, '*');
                    tostring_(root->op.r, buf);
                    bufaddc(buf, ')');
                }
                else {
                    tostring_(root->op.l, buf);
                    bufaddc(buf, '*');
                    tostring_(root->op.r, buf);
                }
                break;
            case OP_DIV:
                tostring_(root->op.l, buf);
                bufaddc(buf, '/');
                tostring_(root->op.r, buf);
                break;
            case OP_MOD:
                bufaddc(buf, '(');
                tostring_(root->op.l, buf);
                bufaddstr(buf, " mod ", sizeof(" mod ") - 1);
                tostring_(root->op.r, buf);
                bufaddc(buf, ')');
                break;
            case OP_EXP:
                tostring_(root->op.l, buf);
                bufaddc(buf, '^');
                tostring_(root->op.r, buf);
                break;
            case OP_SIN:
                bufaddstr(buf, "sin(", sizeof("sin(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_COS:
                bufaddstr(buf, "cos(", sizeof("cos(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_TAN:
                bufaddstr(buf, "tan(", sizeof("tan(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_ARCSIN:
                bufaddstr(buf, "arcsin(", sizeof("arcsin(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_ARCCOS:
                bufaddstr(buf, "arccos(", sizeof("arccos(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_ARCTAN:
                bufaddstr(buf, "arctan(", sizeof("arctan(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_SINH:
                bufaddstr(buf, "sinh(", sizeof("sinh(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_COSH:
                bufaddstr(buf, "cosh(", sizeof("cosh(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_TANH:
                bufaddstr(buf, "tanh(", sizeof("tanh(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_LOG:
                bufaddstr(buf, "log(", sizeof("log(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_LN:
                bufaddstr(buf, "ln(", sizeof("ln(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_LG:
                bufaddstr(buf, "lg(", sizeof("lg(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_SQRT:
                bufaddstr(buf, "sqrt(", sizeof("sqrt(") - 1);
                tostring_(root->op.c, buf);
                bufaddc(buf, ')');
                break;
            case OP_NEGATE:
                bufaddc(buf, '-');
                tostring_(root->op.c, buf);
                break;
            default:
                break;
        }
    }
}

