/*
 <statementlist> := <statement> <statementlist'>
 <statementlist> := <statement> <statementlist> | E
 
 <statement> :=  <expression> | <control> | <dec> | return <expression>
 
 <expression> := <simple_expression> <expression'>
 
 <expression'> := relop <simple_expression>
 |
 E
 
 <simple_expression> := <sign> <term> <simple_expression'>
 |
 <term> <simple_expression'>
 
 <simple_expression'> := addop <term> <simple_expression>
 |
 E
 
 <term> := <factor> <term'>
 
 <term'> := mulop <factor> <term'>
 |
 E
 
 <factor> := <factor'> <optexp>
 
 <optexp> := ^ <expression>
 
 <factor'> := id <factor''>
 |
 num
 |
 ( <expression> )
 |
 not <factor>
 |
 string
 |
 regex
 |
 <initializer>
 |
 @ (<paramlist>) { <statementlist> } <optcall>
 
 <factor''> := [ <expression> ] <factor''> | . id <factor''> | ( <arglist> ) | E
 
 <optcall> := ( <arglist> ) | E
 
 <sign> + | -
 
 
 <arglist> := <expression> <arglist'>
 <arglist'> := , <expression> <arglist'> | E
 
 <control> := if <expression> then <statementlist> <elseif>
 |
 while <expression> do <statementlist> endwhile
 |
 for id <- <expression> do <statementlist> endfor
 
 
 <elseif> := else <statementlist> endif | elif <expression> then <statementlist> endif
 
 <dec> := var id : <opttype> := <expression>
 
 <opttype> := <type> | E
 
 <type> := void | _int <array> | _real <array> | _string <array> | _regex <array> | ( <optparamlist> ) -> <type>
 
 <array> := [ <expression> ] <array> | E
 
 <optparamlist> := <paramlist> | E
 <paramlist> := <dec> <paramlist'>
 <paramlist'> := , <dec> <paramlist'>
 
 <initializer> := { <paramlist> <optmap> }
 
 <optmap> := -> <expression>
 
 */

#include "parse.h"
#include "general.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define TOKCHUNK_SIZE 16

enum {
    TOKTYPE_IDENT,
    TOKTYPE_NUM,
    TOKTYPE_STRING,
    TOKTYPE_COMMA,
    TOKTYPE_DOT,
    TOKTYPE_RANGE,
    TOKTYPE_LAMBDA,
    TOKTYPE_MAP,
    TOKTYPE_FORRANGE,
    TOKTYPE_OPENPAREN,
    TOKTYPE_CLOSEPAREN,
    TOKTYPE_OPENBRACKET,
    TOKTYPE_CLOSEBRACKET,
    TOKTYPE_OPENBRACE,
    TOKTYPE_CLOSEBRACE,
    TOKTYPE_EXPOP,
    TOKTYPE_MULOP,
    TOKTYPE_ADDOP,
    TOKTYPE_RELOP,
    TOKTYPE_REGEX,
    TOKTYPE_ASSIGN,
    TOKTYPE_COLON
};

enum {
    TOKATT_DEFAULT,
    TOKATT_NUMREAL,
    TOKATT_NUMINT,
    TOKATT_EQ,
    TOKATT_NEQ,
    TOKATT_GEQ,
    TOKATT_LEQ,
    TOKATT_G,
    TOKATT_L,
    TOKATT_MULT,
    TOKATT_AND,
    TOKATT_DIV,
    TOKATT_MOD,
    TOKATT_OR,
    TOKATT_ADD,
    TOKATT_SUB,
    TOKATT_EXP
};

typedef struct tok_s tok_s;
typedef struct tokchunk_s tokchunk_s;

struct tok_s
{
    uint8_t type;
    uint8_t att;
    char *lex;
};

struct tokchunk_s
{
    uint8_t size;
    tok_s tok[TOKCHUNK_SIZE];
    tokchunk_s *next;
};

static tokchunk_s *lex(char *src);
static void tok(tokchunk_s **list, char *lexeme, size_t len, uint8_t type, uint8_t att);
static void printtoks(tokchunk_s *list);

void parse(char *src)
{
    tokchunk_s *tok;
    
    tok = lex(src);
    printtoks(tok);

}

tokchunk_s *lex(char *src)
{
    char *bptr, c;
    tokchunk_s *head, *curr;
    
    curr = head = alloc(sizeof *head);
    
    head->size = 0;
    head->next = NULL;
    
    while(*src) {
        switch(*src) {
            case ' ':
            case '\t':
            case '\v':
            case '\n':
            case '\r':
                src++;
                break;
            case ',':
                tok(&curr, ",", 1, TOKTYPE_COMMA, TOKATT_DEFAULT);
                src++;
                break;
            case '.':
                tok(&curr, ".", 1, TOKTYPE_DOT, TOKATT_DEFAULT);
                src++;
                break;
            case '(':
                tok(&curr, "(", 1, TOKTYPE_OPENPAREN, TOKATT_DEFAULT);
                src++;
                break;
            case ')':
                tok(&curr, ")", 1, TOKTYPE_CLOSEPAREN, TOKATT_DEFAULT);
                src++;
                break;
            case '[':
                tok(&curr, "[", 1, TOKTYPE_OPENBRACKET, TOKATT_DEFAULT);
                src++;
                break;
            case ']':
                tok(&curr, "]", 1, TOKTYPE_CLOSEBRACKET, TOKATT_DEFAULT);
                src++;
                break;
            case '{':
                tok(&curr, "{", 1, TOKTYPE_OPENBRACE, TOKATT_DEFAULT);
                src++;
                break;
            case '}':
                tok(&curr, "}", 1, TOKTYPE_CLOSEBRACE, TOKATT_DEFAULT);
                src++;
                break;
            case '@':
                tok(&curr, "@", 1, TOKTYPE_LAMBDA, TOKATT_DEFAULT);
                src++;
                break;
            case '<':
                if(*(src + 1) == '-') {
                    tok(&curr, "<-", 2, TOKTYPE_FORRANGE, TOKATT_DEFAULT);
                    src += 2;
                }
                else if(*(src + 1) == '>') {
                    tok(&curr, "<>", 2, TOKTYPE_RELOP, TOKATT_NEQ);
                    src += 2;
                }
                else if(*(src + 1) == '=') {
                    tok(&curr, "<=", 2, TOKTYPE_RELOP, TOKATT_LEQ);
                    src += 2;
                }
                else {
                    tok(&curr, "<", 1, TOKTYPE_RELOP, TOKATT_L);
                    src++;
                }
                break;
            case '>':
                if(*(src + 1) == '=') {
                    tok(&curr, ">=", 2, TOKTYPE_RELOP, TOKATT_GEQ);
                    src += 2;
                }
                else {
                    tok(&curr, ">", 1, TOKTYPE_RELOP, TOKATT_G);
                    src++;
                }
                break;
            case '+':
                if(*(src + 1) == '=') {
                    tok(&curr, "+=", 2, TOKTYPE_ASSIGN, TOKATT_ADD);
                    src++;
                }
                else {
                    tok(&curr, "}", 1, TOKTYPE_ADDOP, TOKATT_ADD);
                    src++;
                }
                break;
            case '-':
                if(*(src + 1) == '>') {
                    tok(&curr, "->", 2, TOKTYPE_MAP, TOKATT_DEFAULT);
                    src += 2;
                }
                else if(*(src + 1) == '=') {
                    tok(&curr, "-=", 2, TOKTYPE_ASSIGN, TOKATT_SUB);
                    src += 2;
                }
                else {
                    tok(&curr, "-", 1, TOKTYPE_ADDOP, TOKATT_SUB);
                    src++;
                }
                break;
            case '*':
                if(*(src + 1) == '=') {
                    tok(&curr, "*=", 2, TOKTYPE_ASSIGN, TOKATT_MULT);
                    src += 2;
                }
                else {
                    tok(&curr, "*", 1, TOKTYPE_MULOP, TOKATT_MULT);
                    src++;
                }
                break;
            case '/':
                if(*(src + 1) == '=') {
                    tok(&curr, "/=", 2, TOKTYPE_ASSIGN, TOKATT_DIV);
                    src += 2;
                }
                else {
                    tok(&curr, "/", 1, TOKTYPE_MULOP, TOKATT_DIV);
                    src++;
                }
                break;
            case '%':
                if(*(src + 1) == '=') {
                    tok(&curr, "%=", 2, TOKTYPE_ASSIGN, TOKATT_MOD);
                    src += 2;
                }
                else {
                    tok(&curr, "%", 1, TOKTYPE_MULOP, TOKATT_MOD);
                    src++;
                }
                break;
            case ':':
                if(*(src + 1) == '=') {
                    tok(&curr, ":=", 2, TOKTYPE_ASSIGN, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    tok(&curr, ":", 1, TOKTYPE_COLON, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case '^':
                if(*(src + 1) == '=') {
                    tok(&curr, "^=", 2, TOKTYPE_ASSIGN, TOKATT_EXP);
                    src += 2;
                }
                else {
                    tok(&curr, "^", 1, TOKTYPE_EXPOP, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case '=':
                if(*(src + 1) == '>') {
                    tok(&curr, "=>", 2, TOKTYPE_RANGE, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    tok(&curr, "=", 1, TOKTYPE_RELOP, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case '"':
                bptr = src++;
                while(*src != '"') {
                    if(*src)
                        src++;
                    else {
                        //badly formed string error
                    }
                }
                tok(&curr, "=", 1, TOKTYPE_RELOP, TOKATT_DEFAULT);
                c = *src;
                *src = '\0';
                tok(&curr, bptr, src - bptr, TOKTYPE_STRING, TOKATT_DEFAULT);
                *src = c;
                break;
            case '#':
                bptr = src++;
                while(*src != '#') {
                    if(*src)
                        src++;
                    else {
                        //badly formed regex error
                    }
                }
                c = *src;
                *src = '\0';
                tok(&curr, bptr, src - bptr, TOKTYPE_REGEX, TOKATT_DEFAULT);
                *src = c;
                break;
            default:
                if(isdigit(*src)) {
                    bool gotdot = false;
                    bptr = src++;
                    while(true) {
                        if(isdigit(*src))
                            src++;
                        else if(*src == '.') {
                            if(!gotdot) {
                                gotdot = true;
                                src++;
                            }
                            else {
                                //lexical error
                                src++;
                            }
                        }
                        else {
                            c = *src;
                            *src = '\0';
                            
                            if(gotdot)
                                tok(&curr, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMREAL);
                            else
                                tok(&curr, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMINT);
                            *src = c;
                            break;
                        }
                    }
                }
                else if(isalpha(*src) || *src == '_') {
                    bptr = src++;
                    while(isalnum(*src) || *src == '_')
                        src++;
                    c = *src;
                    *src = '\0';

                    tok(&curr, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMINT);
                    *src = c;
                }
                else {
                    //lexical error
                }
                    
                break;
        }
    }
    return head;
}

void tok(tokchunk_s **list, char *lexeme, size_t len, uint8_t type, uint8_t att)
{
    char *lex;
    tokchunk_s *l = *list;
    int size = l->size;
    
    if(size == TOKCHUNK_SIZE) {
        l->next = alloc(sizeof *l);
        l = l->next;
        l->size = size = 0;
        l->next = NULL;
        *list = l;
    }
    
    lex = alloc(len + 1);
    strcpy(lex, lexeme);
    l->tok[size].lex = lex;
    l->tok[size].type = type;
    l->tok[size].att = att;
    l->size++;
}

void printtoks(tokchunk_s *list)
{
    int i;
    
    while(list) {
        for(i = 0; i < list->size; i++) {
            printf("%s %d %d\n", list->tok[i].lex, list->tok[i].type, list->tok[i].att);
        }
        list = list->next;
    }
}
