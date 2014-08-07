/*
 <statementlist> := <statement> <statementlist'>
 <statementlist> := <statement> <statementlist> | E
 
 <statement> :=  <expressionlist> | <control> | <dec> | return <expression>
 
 <expressionlist> := <expression> <expressionlist'>
 
 <expressionlist'> := <expression> <expressionlist'> | E
 
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
 
 <factor> := id <factor'>
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
 
 <factor'> := [ <expression> ] | . id <factor'> | ( <paramlist> ) | E
 
 <optcall> := ( <paramlist> ) | E
 
 <sign> + | -
 
 
 <paramlist> := <expression> <paramlist'>
 <paramlist'> := , <expression> <paramlist'> | E
 
 <control> := if <expression> then <statementlist> <elseif>
 |
 while <expression> do <statementlist> endwhile
 |
 for id <- id do <statementlist> endfor
 
 <elseif> := else <statementlist> endif | elif <expression> then <statementlist> endif
 
 <dec> := var id := <expression>
 
 <initializer> := { <paramlist> <optmap> }
 
 <optmap> := -> <expression>
 
 */

#include "parse.h"
#include "general.h"
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
    TOKTYPE_LAMBDA,
    TOKTYPE_MAP,
    TOKTYPE_FMAP,
    TOKTYPE_OPENPAREN,
    TOKTYPE_CLOSEPAREN,
    TOKTYPE_OPENBRACKET,
    TOKTYPE_CLOSEBRACKET,
    TOKTYPE_OPENBRACE,
    TOKTYPE_CLOSEBRACE,
    TOKTYPE_MULOP,
    TOKTYPE_ADDOP,
    TOKTYPE_RELOP,
    TOKTYPE_REGEX
};

enum {
    TOKATT_DEFAULT,
    TOKATT_NUMREAL,
    TOKATT_NUMINT,
    TOKATT_MULT,
    TOKATT_AND,
    TOKATT_DIV,
    TOKATT_MOD,
    TOKATT_OR,
    TOKATT_ADD,
    TOKATT_SUB
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

tokchunk_s *lex(char *src)
{
    char *bptr, c;
    tokchunk_s *head, *curr;
    
    curr = head = alloc(sizeof *head);
    
    head->size = 0;
    head->next = NULL;
    
    while(*src) {
        switch(*src) {
            case ',':
                tok(&curr, ",", 1, TOKTYPE_COMMA, TOKATT_DEFAULT);
                src++;
                break;
            case '.':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '@':
            case '<':
            case '>':
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case ':':
            case '=':
            case '#':
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
                        }
                    }
                }
                else if(isalpha(*src) || *src == '_') {
                    bptr = src++;
                    while(isalpha(*src) || *src == '_')
                        src++;
                    c = *src;
                    *src = '\0';

                    tok(&curr, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMINT);
                    *src = c;
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
