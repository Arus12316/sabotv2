#include "calc.h"
#include "general.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct tok_s tok_s;

typedef enum {
    CALCTOK_MULOP,
    CALCTOK_ADDOP,
    CALCTOK_IDENT,
    CALCTOK_NUM,
    CALCTOK_EXPON,
    CALCTOK_OPENPAREN,
    CALCTOK_CLOSEPAREN
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

/*
 
 
 
 */

static tok_s *lex(char *src);
static void mktok(tok_s **list, char *lex, ttype_e type, ttatt_e);
static void print_tok(tok_s *tok);

void eval(char *exp)
{
    tok_s *list;
    
    char *bck = alloc(strlen(exp));
    strcpy(bck, exp);
    
    list = lex(bck);
    print_tok(list);
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
                mktok(&curr, "(", CALCTOK_EXPON, CALCATT_DEFAULT);
                src++;
                break;
            case ')':
                mktok(&curr, ")", CALCTOK_EXPON, CALCATT_DEFAULT);
                src++;
                break;
            default:
                if(isalpha(*src) || *src == '-') {
                    bptr = src;
                    while(isalnum(*++src) || *src == '_');
                    if(!strcmp(bptr, "plus")) {
                        mktok(&curr, "+", CALCTOK_ADDOP, CALCATT_ADD);
                    }
                    else {
                        bck = *src;
                        *src = '\0';
                        mktok(&curr, bptr, CALCTOK_IDENT, CALCATT_DEFAULT);
                        *src = bck;
                    }
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
                        while(isdigit(*src) || *src == '.');
                    }
                    
                    bck = *src;
                    *src = '\0';
                    mktok(&curr, bptr, CALCTOK_IDENT, CALCATT_DEFAULT);
                    *src = bck;
                }
                else {
                    fprintf(stderr, "Lexical Error: Unknown Symbol %c\n", *src);
                    src++;
                }
                break;
        }
    }
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

