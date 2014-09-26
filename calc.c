#include "calc.h"
#include "general.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define TOK() (*tok)
#define NEXTTOK() (*tok = (*tok)->next)

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

static tok_s *lex(char *src);
static void mktok(tok_s **list, char *lex, ttype_e type, ttatt_e);
static void print_tok(tok_s *tok);

/*
 
 <start> -> <expression> | ε
 
 <expression> -> <term> <expression'>
 
 <expression'> -> addop <term> <expression'> | ε
 
 <term> -> <subterm> <term'>
 
 <term'> -> mulop <subterm> <term'> | ε
 
 <subterm> -> <factor> <subterm'>
 
 <subterm'> -> expon <factor> <subterm'> | ε
 
 <factor> -> num | ident | ( <expression> ) | addop <factor>
 
 
 */
static void p_start(tok_s **tok);
static double p_expression(tok_s **tok);
static void p_expression_(tok_s **tok, double *accum);
static double p_term(tok_s **tok);
static void p_term_(tok_s **tok, double *accum);
static double p_subterm(tok_s **tok);
static double p_subterm_(tok_s **tok);
static double p_factor(tok_s **tok);

void eval(char *exp)
{
    tok_s *list, *start;
    
    char *bck = alloc(strlen(exp));
    strcpy(bck, exp);
    
    list = lex(bck);
    start = list->next;
    p_start(&start);
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
                                    fprintf(stderr, "Lexical Error: Improperly formed number in exponent\n");
                                    break;
                                }
                                gotdec = true;
                            }
                            else {
                                if(*(src - 1) == '.' && !isdigit((*src - 2))) {
                                    fprintf(stderr, "Lexical Error: Improperly formed number in exponent\n");
                                }
                                else if(*(src - 1) == '-' || *(src - 1) == '+') {
                                    fprintf(stderr, "Lexical Error: Improperly formed number in exponent\n");
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

void p_start(tok_s **tok)
{
    double result;
    tok_s *t = TOK();
    
    if(t->type != CALCTOK_EOF) {
        result = p_expression(tok);
        t = TOK();
        if(t->type == CALCTOK_EOF) {
            printf("parse success!\nresult: %f\n", result);
            
        }
        else {
            fprintf(stderr, "Syntax Error: Expected EOF but got %s\n", t->lex);
        }
        
    }
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
    tok_s *t = TOK(), *bck;
    
    if(t->type == CALCTOK_MULOP) {
        bck = t;
        NEXTTOK();
        subterm = p_subterm(tok);
        if(bck->att == CALCATT_MULT) {
            *accum *= subterm;
        }
        else {
            *accum /= subterm;
        }
        p_term_(tok, accum);
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
            NEXTTOK();
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
            NEXTTOK();
            return 0;
    }
}

