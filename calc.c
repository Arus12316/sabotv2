#include "calc.h"
#include "general.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>

#define TOK() (*tok)
#define NEXTTOK() (*tok = (*tok)->next)

#define TC(C) TCC(C)
#define TCC(C) #C


typedef struct tok_s tok_s;

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


struct tok_s {
    char *lex;
    ttype_e type;
    ttatt_e att;
    tok_s *next;
};

typedef struct opnode_s opnode_s;
typedef struct numnode_s numnode_s;
typedef struct idnode_s idnode_s;
typedef struct node_s node_s;

struct opnode_s
{
    tok_s *t;
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
    enum {
        NTYPE_OP,
        NTYPE_NUM,
        NTYPE_ID
    } type;
    union {
        opnode_s op;
        numnode_s num;
        idnode_s ident;
    };
};

static tok_s *lex(char *src);
static void mktok(tok_s **list, char *lex, ttype_e type, ttatt_e);
static void print_tok(tok_s *tok);
static void free_tok(tok_s *tok);

/*

 <start> -> <expression> | ε
 
 <expression> -> <term> <expression'>
 
 <expression'> -> addop <term> <expression'> | ε
 
 <term> -> <subterm> <term'>
 
 <term'> -> mulop <subterm> <term'> | ( <expression>) <parenfollow> | ε

 <parenfollow> -> <term> | <term'>
 
 <subterm> -> <factor> <subterm'>
 
 <subterm'> -> expon <factor> <subterm'> | ε
 
 <factor> -> num | ident <func> | ( <expression> ) | addop <factor>
 
 <func> -> (<paramlist>) | ε
 
 <paramlist> -> <expression> <paramlist'> | ε
 
 <paramlist'> -> , <expression>
 
 */

static int status;

static calcres_s p_start(tok_s **tok);
static double p_expression(tok_s **tok);
static void p_expression_(tok_s **tok, double *accum);
static double p_term(tok_s **tok);
static void p_term_(tok_s **tok, double *accum);
static void p_parenfollow(tok_s **tok, double *accum);
static double p_subterm(tok_s **tok);
static double p_subterm_(tok_s **tok);
static double p_factor(tok_s **tok);

calcres_s eval(char *exp)
{
    tok_s *list, *start;
    calcres_s res;

    char *bck = alloc(strlen(exp));
    strcpy(bck, exp);
    
    list = lex(bck);
    start = list->next;
    res = p_start(&start);
    free_tok(list);
    free(bck);
    return res;
}

tok_s *lex(char *src)
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
                            status = 1;
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
                                    fprintf(stderr, "Lexical Error: Improperly formed number in exponent\n");
                                    status = 1;
                                    break;
                                }
                                gotdec = true;
                            }
                            else {
                                if(*(src - 1) == '.' && !isdigit((*src - 2))) {
                                    fprintf(stderr, "Lexical Error: Improperly formed number in exponent\n");
                                    status = 1;
                                }
                                else if(*(src - 1) == '-' || *(src - 1) == '+') {
                                    fprintf(stderr, "Lexical Error: Improperly formed number in exponent\n");
                                    status = 1;
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
                    fprintf(stderr, "Lexical Error: Unknown Symbol %c\n", *src);
                    status = 1;
                    src++;
                }
                break;
        }
    }
    mktok(&curr, "EOF", CALCTOK_EOF, CALCATT_DEFAULT);
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
    
    while(tok) {
        bck = tok->next;
        free(tok);
        tok = bck;
    }
}

calcres_s p_start(tok_s **tok)
{
    double result;
    calcres_s cres;
    tok_s *t = TOK();
    
    if(t->type != CALCTOK_EOF) {
        result = p_expression(tok);
        t = TOK();
        if(t->type == CALCTOK_EOF) {
            printf("parse success!\nresult: %f\n", result);
            
        }
        else {
            fprintf(stderr, "Syntax Error: Expected EOF but got %s\n", t->lex);
            status = 1;
        }
    }
    else {
        result = 0;
        status = 1;
    }
    cres.status = status;
    cres.val = result;
    status = 0;
    return cres;
}

double p_expression(tok_s **tok)
{
    double accum;
    
    accum = p_term(tok);
    p_expression_(tok, &accum);
    return accum;
}

void p_expression_(tok_s **tok, double *accum)
{
    double term;
    tok_s *t = TOK(), *bck;
    
    if(t->type == CALCTOK_ADDOP) {
        bck = t;
        NEXTTOK();
        term = p_term(tok);
        if(bck->att == CALCATT_ADD) {
            *accum += term;
        }
        else {
            *accum -= term;
        }
        p_expression_(tok, accum);
    }
}

double p_term(tok_s **tok)
{
    double accum;
    
    accum = p_subterm(tok);
    p_term_(tok, &accum);
    return accum;
}

void p_term_(tok_s **tok, double *accum)
{
    double subterm;
    long long iterm;
    tok_s *t = TOK(), *bck;
    
    if(t->type == CALCTOK_MULOP) {
        bck = t;
        NEXTTOK();
        subterm = p_subterm(tok);
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
        p_term_(tok, accum);
    }
    else if(t->type == CALCTOK_OPENPAREN) {
        NEXTTOK();
        subterm = p_expression(tok);
        t = TOK();
        if(t->type == CALCTOK_CLOSEPAREN) {
            NEXTTOK();
            *accum *= subterm;
            p_parenfollow(tok, accum);
        }
        else {
            fprintf(stderr, "Syntax Error: Expected ) but got %s\n", t->lex);
        }
    }
}

void p_parenfollow(tok_s **tok, double *accum)
{
    tok_s *t = TOK();
    double term;
    
    switch(t->type) {
        case CALCTOK_NUM:
        case CALCTOK_IDENT:
        case CALCTOK_OPENPAREN:
        case CALCTOK_ADDOP:
            term = p_term(tok);
            *accum *= term;
            break;
        default:
            p_term_(tok, accum);
            break;
    }
}

double p_subterm(tok_s **tok)
{
    double accum, subterm_;
    
    accum = p_factor(tok);
    subterm_ = p_subterm_(tok);
    return pow(accum, subterm_);
}

double p_subterm_(tok_s **tok)
{
    double factor, subterm_;
    tok_s *t = TOK();
    
    if(t->type == CALCTOK_EXPON) {
        NEXTTOK();
        factor = p_factor(tok);
        subterm_ = p_subterm_(tok);
        return pow(factor, subterm_);
    }
    return 1;
}

double p_factor(tok_s **tok)
{
    int mult;
    double exp;
    tok_s *t = TOK(), *bck;
    
    switch(t->type) {
        case CALCTOK_NUM:
            bck = t;
            NEXTTOK();
            return atof(bck->lex);
        case CALCTOK_IDENT:
            bck = t;
            NEXTTOK();
            t = TOK();
            if(t->type == CALCTOK_OPENPAREN) {
                NEXTTOK();
                exp = p_expression(tok);
                t = TOK();
                if(t->type != CALCTOK_CLOSEPAREN) {
                    fprintf(stderr, "Syntax Error: Expected ) but got %s\n", t->lex);
                }
                NEXTTOK();
                if(!strcmp(bck->lex, "sin")) {
                    return sin(exp);
                }
                else if(!strcmp(bck->lex, "cos")) {
                    return cos(exp);
                }
                else if(!strcmp(bck->lex, "tan")) {
                    return tan(exp);
                }
                else if(!strcmp(bck->lex, "arcsin")) {
                    return asin(exp);
                }
                else if(!strcmp(bck->lex, "arccos")) {
                    return acos(exp);
                }
                else if(!strcmp(bck->lex, "arctan")) {
                    return atan(exp);
                }
                else if(!strcmp(bck->lex, "log")) {
                    return log10(exp);
                }
                else if(!strcmp(bck->lex, "ln")) {
                    return log(exp);
                }
                else if(!strcmp(bck->lex, "lg")) {
                    return log2(exp);
                }
                else if(!strcmp(bck->lex, "sqrt")) {
                    return sqrt(exp);
                }
            }
            return 0;
        case CALCTOK_OPENPAREN:
            NEXTTOK();
            exp = p_expression(tok);
            t = TOK();
            if(t->type == CALCTOK_CLOSEPAREN) {
                NEXTTOK();
            }
            else {
                fprintf(stderr, "Syntax Error: Expected ) but got %s\n", t->lex);
                status = 1;
                NEXTTOK();
            }
            return exp;
        case CALCTOK_ADDOP:
            if(t->att == CALCATT_SUB)
                mult = -1;
            else
                mult = 1;
            NEXTTOK();
            return mult*p_factor(tok);
            break;
        default:
            fprintf(stderr, "Syntax Error: Expected number, identifier, (, +, or -, but got %s\n", t->lex);
            status = 1;
            NEXTTOK();
            return 0;
    }
}

