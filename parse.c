/*
 <statementlist> -> <statement> <statementlist'>
 
 <statementlist'> -> <statement> <statementlist'> | ε
 
 <statement> ->  <expression> ; | <control> | <dec> ; | return <expression> ;
 
 <expression> -> <simple_expression> <expression'>
 
 <expression'> -> relop <simple_expression> | ε
 
 <simple_expression> -> <sign> <term> <simple_expression'>
                        |
                        <term> <simple_expression'>
 
 <simple_expression'> -> addop <term> <simple_expression'> | ε
 
 <term> -> <factor> <term'>
 
 <term'> -> mulop <factor> <term'> | ε
 
 <factor> -> <factor'> <optexp>
 
 <optexp> -> ^ <expression> | ε
 
 <factor'> ->   id <factor''> <assign>
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
                <set>
                |
                <lambda>
 
 
 <factor''> -> [ <expression> ] <factor''> | . id <factor''> | ( <arglist> ) | ε
 
 <assign> -> assignop <expression> | ε
 
 <lambda> -> @ (<paramlist>) openbrace <statementlist> closebrace
 
 <sign> -> + | -
 
 
 <arglist> -> <expression> <arglist'>
 <arglist'> -> , <expression> <arglist'> | ε
 
 <control> ->   if <expression> then <statementlist> <elseif>
                |
                while <expression> do <statementlist> endwhile
                |
                for id <- <expression> do <statementlist> endfor
                |
                switch(<expression>) <caselist> endswitch
 

 <caselist> ->  
                case <arglist> map <expression> <caselist>
                | 
                default map <expression> <caselist>
                |
                ε


 <elseif> -> else <statementlist> endif | elif <expression> then <statementlist> endif
 
 <dec> -> var id : <opttype> <assign>
 
 <opttype> -> <type> | ε
 
 <type> -> void | integer <array> | real <array> | String <array> | Regex <array> | ( <optparamlist> ) map <type>
 
 <array> -> [ <expression> ] <array> | ε
 
 <optparamlist> -> <paramlist> | ε
 <paramlist> -> <dec> <paramlist'>
 <paramlist'> -> , <dec> <paramlist'>
 
 <set> -> openbrace <expression> <optnext> <set'> closebrace
 
 <optnext> -> map <expression> | range <expression> | ε
 
 <set'> -> , <expression> <optnext> <set'> | ε
 
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
    TOKTYPE_EOF,
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
    TOKTYPE_COLON,
    TOKTYPE_SEMICOLON,
    TOKTYPE_NOT,
    TOKTYPE_IF,
    TOKTYPE_THEN,
    TOKTYPE_ELIF,
    TOKTYPE_ELSE,
    TOKTYPE_ENDIF,
    TOKTYPE_SWITCH,
    TOKTYPE_ENDSWITCH,
    TOKTYPE_CASE,
    TOKTYPE_DEFAULT,
    TOKTYPE_WHILE,
    TOKTYPE_ENDWHILE,
    TOKTYPE_DO,
    TOKTYPE_FOR,
    TOKTYPE_ENDFOR,
    TOKTYPE_VAR,
    TOKTYPE_VOID,
    TOKTYPE_INTEGER,
    TOKTYPE_REAL,
    TOKTYPE_STRINGTYPE,
    TOKTYPE_REGEXTYPE,
    TOKTYPE_RETURN
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
typedef struct tokiter_s tokiter_s;

struct tok_s
{
    uint8_t type;
    uint8_t att;
    uint16_t line;
    char *lex;
    
};

struct tokchunk_s
{
    uint8_t size;
    tok_s tok[TOKCHUNK_SIZE];
    tokchunk_s *next;
};

struct tokiter_s
{
    uint8_t i;
    tokchunk_s *curr;
};

static tokchunk_s *lex(char *src);
static void mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att);
static void printtoks(tokchunk_s *list);

static tok_s *tok(tokiter_s *ti);
static tok_s *nexttok(tokiter_s *ti);

static void start(tokchunk_s *tokens);
static void p_statementlist(tokiter_s *ti);
static void p_statement(tokiter_s *ti);
static void p_expression(tokiter_s *ti);
static void p_expression_(tokiter_s *ti);
static void p_simple_expression(tokiter_s *ti);
static void p_simple_expression_(tokiter_s *ti);
static void p_term(tokiter_s *ti);
static void p_term_(tokiter_s *ti);
static void p_factor(tokiter_s *ti);
static void p_optexp(tokiter_s *ti);
static void p_factor_(tokiter_s *ti);
static void p_factor__(tokiter_s *ti);
static void p_assign(tokiter_s *ti);
static void p_lambda(tokiter_s *ti);
static void p_sign(tokiter_s *ti);
static void p_arglist(tokiter_s *ti);
static void p_arglist_(tokiter_s *ti);
static void p_control(tokiter_s *ti);
static void p_caselist(tokiter_s *ti);
static void p_elseif(tokiter_s *ti);
static void p_dec(tokiter_s *ti);
static void p_opttype(tokiter_s *ti);
static void p_type(tokiter_s *ti);
static void p_array(tokiter_s *ti);
static void p_optparamlist(tokiter_s *ti);
static void p_paramlist(tokiter_s *ti);
static void p_paramlist_(tokiter_s *ti);
static void p_set(tokiter_s *ti);
static void p_optnext(tokiter_s *ti);
static void p_set_(tokiter_s *ti);


void parse(char *src)
{
    tokchunk_s *tok;
    
    tok = lex(src);
    printtoks(tok);
    
    start(tok);

}

tokchunk_s *lex(char *src)
{
    uint16_t line = 0;
    char *bptr, c;
    tokchunk_s *head, *curr;
    
    curr = head = alloc(sizeof *head);
    
    head->size = 0;
    head->next = NULL;
    
    while(*src) {
        switch(*src) {
            case '\n':
                line++;
            case ' ':
            case '\t':
            case '\v':
            case '\r':
                src++;
                break;
            case ',':
                mtok(&curr, line, ",", 1, TOKTYPE_COMMA, TOKATT_DEFAULT);
                src++;
                break;
            case '.':
                mtok(&curr, line, ".", 1, TOKTYPE_DOT, TOKATT_DEFAULT);
                src++;
                break;
            case '(':
                mtok(&curr, line, "(", 1, TOKTYPE_OPENPAREN, TOKATT_DEFAULT);
                src++;
                break;
            case ')':
                mtok(&curr, line, ")", 1, TOKTYPE_CLOSEPAREN, TOKATT_DEFAULT);
                src++;
                break;
            case '[':
                mtok(&curr, line, "[", 1, TOKTYPE_OPENBRACKET, TOKATT_DEFAULT);
                src++;
                break;
            case ']':
                mtok(&curr, line, "]", 1, TOKTYPE_CLOSEBRACKET, TOKATT_DEFAULT);
                src++;
                break;
            case '{':
                mtok(&curr, line, "{", 1, TOKTYPE_OPENBRACE, TOKATT_DEFAULT);
                src++;
                break;
            case '}':
                mtok(&curr, line, "}", 1, TOKTYPE_CLOSEBRACE, TOKATT_DEFAULT);
                src++;
                break;
            case '@':
                mtok(&curr, line, "@", 1, TOKTYPE_LAMBDA, TOKATT_DEFAULT);
                src++;
                break;
            case '!':
                mtok(&curr, line, "!", 1, TOKTYPE_NOT, TOKATT_DEFAULT);
                src++;
                break;
            case '&':
                mtok(&curr, line, "!", 1, TOKTYPE_MULOP, TOKATT_AND);
                src++;
                break;
            case '|':
                mtok(&curr, line, "!", 1, TOKTYPE_ADDOP, TOKATT_OR);
                src++;
                break;
            case '<':
                if(*(src + 1) == '-') {
                    mtok(&curr, line, "<-", 2, TOKTYPE_FORRANGE, TOKATT_DEFAULT);
                    src += 2;
                }
                else if(*(src + 1) == '>') {
                    mtok(&curr, line, "<>", 2, TOKTYPE_RELOP, TOKATT_NEQ);
                    src += 2;
                }
                else if(*(src + 1) == '=') {
                    mtok(&curr, line, "<=", 2, TOKTYPE_RELOP, TOKATT_LEQ);
                    src += 2;
                }
                else {
                    mtok(&curr, line, "<", 1, TOKTYPE_RELOP, TOKATT_L);
                    src++;
                }
                break;
            case '>':
                if(*(src + 1) == '=') {
                    mtok(&curr, line, ">=", 2, TOKTYPE_RELOP, TOKATT_GEQ);
                    src += 2;
                }
                else {
                    mtok(&curr, line, ">", 1, TOKTYPE_RELOP, TOKATT_G);
                    src++;
                }
                break;
            case '+':
                if(*(src + 1) == '=') {
                    mtok(&curr, line, "+=", 2, TOKTYPE_ASSIGN, TOKATT_ADD);
                    src++;
                }
                else {
                    mtok(&curr, line, "}", 1, TOKTYPE_ADDOP, TOKATT_ADD);
                    src++;
                }
                break;
            case '-':
                if(*(src + 1) == '>') {
                    mtok(&curr, line, "->", 2, TOKTYPE_MAP, TOKATT_DEFAULT);
                    src += 2;
                }
                else if(*(src + 1) == '=') {
                    mtok(&curr, line, "-=", 2, TOKTYPE_ASSIGN, TOKATT_SUB);
                    src += 2;
                }
                else {
                    mtok(&curr, line, "-", 1, TOKTYPE_ADDOP, TOKATT_SUB);
                    src++;
                }
                break;
            case '*':
                if(*(src + 1) == '=') {
                    mtok(&curr, line, "*=", 2, TOKTYPE_ASSIGN, TOKATT_MULT);
                    src += 2;
                }
                else {
                    mtok(&curr, line, "*", 1, TOKTYPE_MULOP, TOKATT_MULT);
                    src++;
                }
                break;
            case '/':
                if(*(src + 1) == '=') {
                    mtok(&curr, line, "/=", 2, TOKTYPE_ASSIGN, TOKATT_DIV);
                    src += 2;
                }
                else {
                    mtok(&curr, line, "/", 1, TOKTYPE_MULOP, TOKATT_DIV);
                    src++;
                }
                break;
            case '%':
                if(*(src + 1) == '=') {
                    mtok(&curr, line, "%=", 2, TOKTYPE_ASSIGN, TOKATT_MOD);
                    src += 2;
                }
                else {
                    mtok(&curr, line, "%", 1, TOKTYPE_MULOP, TOKATT_MOD);
                    src++;
                }
                break;
            case ':':
                if(*(src + 1) == '=') {
                    mtok(&curr, line, ":=", 2, TOKTYPE_ASSIGN, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    mtok(&curr, line, ":", 1, TOKTYPE_COLON, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case ';':
                mtok(&curr, line, ";", 1, TOKTYPE_SEMICOLON, TOKATT_DEFAULT);
                src++;
                break;
            case '^':
                if(*(src + 1) == '=') {
                    mtok(&curr, line, "^=", 2, TOKTYPE_ASSIGN, TOKATT_EXP);
                    src += 2;
                }
                else {
                    mtok(&curr, line, "^", 1, TOKTYPE_EXPOP, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case '=':
                if(*(src + 1) == '>') {
                    mtok(&curr, line, "=>", 2, TOKTYPE_RANGE, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    mtok(&curr, line, "=", 1, TOKTYPE_RELOP, TOKATT_DEFAULT);
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
                c = *src;
                *src = '\0';
                mtok(&curr, line, bptr, src - bptr, TOKTYPE_STRING, TOKATT_DEFAULT);
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
                mtok(&curr, line, bptr, src - bptr, TOKTYPE_REGEX, TOKATT_DEFAULT);
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
                                mtok(&curr, line, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMREAL);
                            else
                                mtok(&curr, line, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMINT);
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

                    mtok(&curr, line, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMINT);
                    *src = c;
                }
                else {
                    //lexical error
                }
                    
                break;
        }
    }
    mtok(&curr, line, "`EOF`", 5, TOKTYPE_EOF, TOKATT_DEFAULT);
    return head;
}

void mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att)
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
    l->tok[size].line = line;
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

tok_s *tok(tokiter_s *ti)
{
    if(ti->curr->size == TOKCHUNK_SIZE) {
        if(ti->curr->next)
            return &ti->curr->next->tok[0];
        else
            return NULL;
    }
    return &ti->curr->tok[ti->i];
}

tok_s *nexttok(tokiter_s *ti)
{
    uint8_t i = ti->i;
    
    if(i < ti->curr->size) {
        ti->i++;
        return &ti->curr->tok[i];
    }
    else if(ti->curr->size == TOKCHUNK_SIZE) {
        ti->curr = ti->curr->next;
        ti->i = 1;
        return &ti->curr->tok[0];
    }
    return NULL;
}

void start(tokchunk_s *tokens)
{
    tokiter_s iter = { .i = 0, .curr = tokens};
    
    p_statementlist(&iter);
}

void p_statementlist(tokiter_s *ti)
{
    if(tok(ti)->type != TOKTYPE_EOF) {
        do {
            p_statement(ti);
        }
        while(tok(ti)->type != TOKTYPE_EOF);
    }
    else {
        //error blank program
    }
}


void p_statement(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
        case TOKTYPE_NUM:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NOT:
        case TOKTYPE_STRING:
        case TOKTYPE_REGEX:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_OPENBRACE:
        case TOKTYPE_ADDOP:
            p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_SEMICOLON) {
                nexttok(ti);
            }
            else {
                //syntax error
            }
            break;
        case TOKTYPE_IF:
        case TOKTYPE_WHILE:
        case TOKTYPE_FOR:
        case TOKTYPE_SWITCH:
            p_control(ti);
            break;
        case TOKTYPE_VAR:
            p_dec(ti);
            if(t->type == TOKTYPE_SEMICOLON) {
                nexttok(ti);
            }
            else {
                //syntax error
            }
            break;
        case TOKTYPE_RETURN:
            nexttok(ti);
            p_expression(ti);
            if(t->type == TOKTYPE_SEMICOLON) {
                nexttok(ti);
            }
            else {
                //syntax error
            }
            break;
        default:
            //syntax error
            break;
    }
}

void p_expression(tokiter_s *ti)
{
    p_simple_expression(ti);
    p_expression_(ti);
}

void p_expression_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_RELOP) {
        nexttok(ti);
        p_simple_expression(ti);
    }
}

void p_simple_expression(tokiter_s *ti)
{
    tok_s *t = tok(ti);

    if(t->type == TOKTYPE_ADDOP)
        p_sign(ti);
    p_term(ti);
    p_simple_expression_(ti);
}

void p_simple_expression_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ADDOP) {
        nexttok(ti);
        p_term(ti);
        p_simple_expression_(ti);
    }
}

void p_term(tokiter_s *ti)
{
    p_factor(ti);
    p_term_(ti);
}

void p_term_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_MULOP) {
        nexttok(ti);
        p_factor(ti);
        p_term_(ti);
    }
}

void p_factor(tokiter_s *ti)
{
    p_factor_(ti);
    p_optexp(ti);
}

void p_optexp(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_EXPOP) {
        nexttok(ti);
        p_expression(ti);
    }
}

void p_factor_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
            nexttok(ti);
            p_factor__(ti);
            break;
        case TOKTYPE_NUM:
            nexttok(ti);
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                
            }
            else {
                //syntax error
            }
            break;
        case TOKTYPE_STRING:
            nexttok(ti);
            break;
        case TOKTYPE_REGEX:
            nexttok(ti);
            break;
        case TOKTYPE_OPENBRACE:
            p_set(ti);
            break;
        case TOKTYPE_LAMBDA:
            p_lambda(ti);
            break;
        default:
            //syntax error
            break;
    }
}

void p_factor__(tokiter_s *ti)
{
    
}

void p_assign(tokiter_s *ti)
{
    
}

void p_lambda(tokiter_s *ti)
{
    
}

void p_sign(tokiter_s *ti)
{
    
}

void p_arglist(tokiter_s *ti)
{
    
}

void p_arglist_(tokiter_s *ti)
{
    
}

void p_control(tokiter_s *ti)
{
    
}

void p_caselist(tokiter_s *ti)
{
    
}

void p_elseif(tokiter_s *ti)
{
    
}

void p_dec(tokiter_s *ti)
{
    
}

void p_opttype(tokiter_s *ti)
{
    
}


void p_type(tokiter_s *ti)
{
    
}

void p_array(tokiter_s *ti)
{
    
}

void p_optparamlist(tokiter_s *ti)
{
    
}

void p_paramlist(tokiter_s *ti)
{
    
}

void p_paramlist_(tokiter_s *ti)
{
    
}

void p_set(tokiter_s *ti)
{
    
}

void p_optnext(tokiter_s *ti)
{
    
}

void p_set_(tokiter_s *ti)
{
    
}

