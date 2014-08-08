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
                <lambda> { <factor'>.type := closure; <factor'>.val := <lambda>.val }
 
 
 
 <factor''> -> [ <expression> ] <factor''> | . id <factor''> | ( <optarglist> ) | ε
 
 <assign> -> assignop <expression> | ε
 
 <lambda> -> @ (<optparamlist>) openbrace <statementlist> closebrace
 
 <sign> -> + | -
 
 <optarglist> -> <arglist> | ε
 <arglist> -> <expression> <arglist'>
 <arglist'> -> , <expression> <arglist'> | ε
 
 <control> ->   if <expression> then <statementlist> <elseif> endif
                |
                while <expression> do <statementlist> endwhile
                |
                for id <- <expression> do <statementlist> endfor
                |
                switch(<expression>) <caselist> endswitch
                |
                listener(<arglist>) openbrace <statementlist> closebrace
 

 <caselist> ->  
                case <arglist> map <expression> <caselist>
                | 
                default map <expression> <caselist>
                |
                ε


 <elseif> -> elif <expression> then <statementlist> | else <statementlist>
 
 <dec> -> var id <opttype> <assign>
 
 <opttype> -> : <type> | ε
 
 <type> -> void | integer <array> | real <array> | String <array> | Regex <array> | set <array> |( <optparamlist> ) <array> map <type>
 
 <array> -> [ <expression> ] <array> | ε
 
 <optparamlist> -> <paramlist> | ε
 <paramlist> -> <dec> <paramlist'>
 <paramlist'> -> , <dec> <paramlist'> | ε
 
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
#include <stdarg.h>

#define TOKCHUNK_SIZE 16
#define MAX_ERRARGS 256

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
    TOKTYPE_FORVAR,
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
    TOKTYPE_LISENER,
    TOKTYPE_VAR,
    TOKTYPE_VOID,
    TOKTYPE_INTEGER,
    TOKTYPE_REAL,
    TOKTYPE_STRINGTYPE,
    TOKTYPE_REGEXTYPE,
    TOKTYPE_SET,
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

static struct tok_s
{
    uint8_t type;
    uint8_t att;
    uint16_t line;
    char *lex;
}
eoftok = {
    TOKTYPE_EOF,
    TOKATT_DEFAULT,
    0,
    "`EOF`"
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
    errlist_s *err;
    errlist_s *ecurr;
};


static struct keyw_s {
    char *str;
    uint8_t type;
}
keywords[] = {
    {"not", TOKTYPE_NOT},
    {"if", TOKTYPE_IF},
    {"then", TOKTYPE_THEN},
    {"elif", TOKTYPE_ELIF},
    {"else", TOKTYPE_ELSE},
    {"endif", TOKTYPE_ENDIF},
    {"while", TOKTYPE_WHILE},
    {"do", TOKTYPE_DO},
    {"endwhile", TOKTYPE_ENDWHILE},
    {"for", TOKTYPE_FOR},
    {"endfor", TOKTYPE_ENDFOR},
    {"switch", TOKTYPE_SWITCH},
    {"case", TOKTYPE_CASE},
    {"default", TOKTYPE_DEFAULT},
    {"endswitch", TOKTYPE_ENDSWITCH},
    {"listener", TOKTYPE_LISENER},
    {"var", TOKTYPE_VAR},
    {"void", TOKTYPE_VOID},
    {"int", TOKTYPE_INTEGER},
    {"real", TOKTYPE_REAL},
    {"string", TOKTYPE_STRINGTYPE},
    {"regex", TOKTYPE_REGEXTYPE},
    {"set", TOKTYPE_SET},
    {"return", TOKTYPE_RETURN}
};

static tokiter_s *lex(char *src);
static void mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att);
static bool trykeyword(tokchunk_s **list, uint16_t line, char *str);
static void printtoks(tokchunk_s *list);

static tok_s *tok(tokiter_s *ti);
static tok_s *nexttok(tokiter_s *ti);

static void start(tokiter_s *ti);
static void p_statementlist(tokiter_s *ti);
static void p_statementlist_(tokiter_s *ti);
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
static void p_optarglist(tokiter_s *ti);
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
static void synerr_rec(tokiter_s *ti);

static void adderr(tokiter_s *ti, char *prefix, char *got, uint16_t line, ...);

errlist_s *parse(char *src)
{
    tokiter_s *ti;
    
    ti = lex(src);
    
    start(ti);
    
    return ti->err;
}

tokiter_s *lex(char *src)
{
    uint16_t line = 1;
    char *bptr, c;
    tokchunk_s *head, *curr;
    tokiter_s *ti;
    
    ti = alloc(sizeof *ti);
    curr = head = alloc(sizeof *head);
    
    head->size = 0;
    head->next = NULL;
    
    ti->err = NULL;
    ti->ecurr = NULL;
    ti->i = 0;
    ti->curr = head;
    
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
                    mtok(&curr, line, "<-", 2, TOKTYPE_FORVAR, TOKATT_DEFAULT);
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
                    src += 2;
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
                        adderr(ti, "Lexical Error", "EOF", line, "null byte", NULL);
                        break;
                    }
                }
                c = *++src;
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
                        adderr(ti, "Lexical Error", "EOF", line, "#", NULL);
                        break;
                    }
                }
                c = *++src;
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
                else if(isalpha(*src) || *src == '_' || *src == '$') {
                    bptr = src++;
                    while(isalnum(*src) || *src == '_' || *src == '$')
                        src++;
                    c = *src;
                    *src = '\0';
                    if(!trykeyword(&curr, line, bptr))
                        mtok(&curr, line, bptr, src - bptr, TOKTYPE_IDENT, TOKATT_DEFAULT);
                    *src = c;
                }
                else {
                    c = *++src;
                    *src = '\0';
                    adderr(ti, "Lexical Error", src - 1, line, "language chararacter", NULL);
                    *src = c;
                }
                break;
        }
    }
    mtok(&curr, line, "`EOF`", 5, TOKTYPE_EOF, TOKATT_DEFAULT);
    
    return ti;
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

bool trykeyword(tokchunk_s **list, uint16_t line, char *str)
{
    int i;
    size_t len;
    
    for(i = 0; i < sizeof(keywords) / sizeof(struct keyw_s); i++) {
        if(!strcmp(str, keywords[i].str)) {
            len = strlen(keywords[i].str);
            mtok(list, line, keywords[i].str, len, keywords[i].type, TOKATT_DEFAULT);
            return true;
        }
    }
    return false;
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
    
    if(ti->i == ti->curr->size) {
        if(ti->curr->next)
            return &ti->curr->next->tok[0];
        else {
            perror("tok peek error");
            return &eoftok;
        }
    }
    return &ti->curr->tok[ti->i];
}

tok_s *nexttok(tokiter_s *ti)
{
    uint8_t i = ti->i;
    
    if(i < ti->curr->size-1) {
        ti->i++;
        return &ti->curr->tok[i + 1];
    }
    else if(ti->i == ti->curr->size - 1) {
        ti->curr = ti->curr->next;
        ti->i = 0;
        return &ti->curr->tok[0];
    }
    perror("nexttok error");
    return &eoftok;
}

void start(tokiter_s *ti)
{
    tok_s *t;
    
    p_statementlist(ti);
    t = tok(ti);
    
    if(t->type != TOKTYPE_EOF) {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "EOF", NULL);
    }
}

void p_statementlist(tokiter_s *ti)
{
    p_statement(ti);
    p_statementlist_(ti);
}

void p_statementlist_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_VAR:
        case TOKTYPE_SWITCH:
        case TOKTYPE_FOR:
        case TOKTYPE_WHILE:
        case TOKTYPE_IF:
        case TOKTYPE_LISENER:
        case TOKTYPE_ADDOP:
        case TOKTYPE_OPENBRACE:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_REGEX:
        case TOKTYPE_STRING:
        case TOKTYPE_NOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NUM:
        case TOKTYPE_IDENT:
        case TOKTYPE_RETURN:
            p_statement(ti);
            p_statementlist_(ti);
            break;
        default:
            //epsilon production
            break;
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
                adderr(ti, "Syntax Error", t->lex, t->line, ";", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_IF:
        case TOKTYPE_WHILE:
        case TOKTYPE_FOR:
        case TOKTYPE_SWITCH:
        case TOKTYPE_LISENER:
            p_control(ti);
            break;
        case TOKTYPE_VAR:
            p_dec(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_SEMICOLON) {
                nexttok(ti);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, ";", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_RETURN:
            nexttok(ti);
            p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_SEMICOLON) {
                nexttok(ti);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, ";", NULL);
                synerr_rec(ti);
            }
            break;
        default:
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", "number", "(", "not", "string",
                   "regex", "@", "{", "+", "-", "|", "if", "while", "for", "switch", "var", "return", NULL);
            synerr_rec(ti);
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
            p_assign(ti);
            break;
        case TOKTYPE_NUM:
            nexttok(ti);
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
                synerr_rec(ti);
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
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", "number", "(", "string", "regex", "{", "@", NULL);
            synerr_rec(ti);
            break;
    }
}

void p_factor__(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_OPENBRACKET:
            nexttok(ti);
            p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACKET) {
                nexttok(ti);
                p_factor__(ti);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "]", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_DOT:
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                nexttok(ti);
                p_factor__(ti);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            p_optarglist(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
                synerr_rec(ti);
            }
            break;
        default:
            //epsilon production
            break;
    }
}

void p_assign(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ASSIGN) {
        nexttok(ti);
        p_expression(ti);
    }
    else {
        //epsilon production
    }
}

void p_lambda(tokiter_s *ti)
{
    //so far, a check that token is '@' is redundant
    tok_s *t = nexttok(ti);
    
    if(t->type == TOKTYPE_OPENPAREN) {
        nexttok(ti);
        p_optparamlist(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEPAREN) {
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                p_statementlist(ti);
                t = tok(ti);
                if(t->type == TOKTYPE_CLOSEBRACE) {
                    nexttok(ti);
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "}", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "{", NULL);
                synerr_rec(ti);
            }
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
            synerr_rec(ti);
        }
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "(", NULL);
        synerr_rec(ti);
    }
    
}

void p_sign(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->att == TOKATT_ADD || t->att == TOKATT_SUB) {
        nexttok(ti);
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "+", "-", NULL);
        synerr_rec(ti);
    }
}

void p_optarglist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_ADDOP:
        case TOKTYPE_OPENBRACE:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_REGEX:
        case TOKTYPE_STRING:
        case TOKTYPE_NOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NUM:
        case TOKTYPE_IDENT:
            p_arglist(ti);
            break;
        default:
            //epsilon production
            break;
    }
}

void p_arglist(tokiter_s *ti)
{
    p_expression(ti);
    p_arglist_(ti);
}

void p_arglist_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        p_expression(ti);
        p_arglist_(ti);
    }
    else {
        //epsilon production
    }
}

void p_control(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IF:
            nexttok(ti);
            p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_THEN) {
                nexttok(ti);
                p_statementlist(ti);
                p_elseif(ti);
                t = tok(ti);
                if(t->type == TOKTYPE_ENDIF) {
                    nexttok(ti);
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "endif", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "then", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_WHILE:
            nexttok(ti);
            p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_DO) {
                nexttok(ti);
                p_statementlist(ti);
                t = tok(ti);
                if(t->type == TOKTYPE_ENDWHILE) {
                    nexttok(ti);
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "endwhile", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "do", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_FOR:
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                t = nexttok(ti);
                if(t->type == TOKTYPE_FORVAR) {
                    nexttok(ti);
                    p_expression(ti);
                    t = tok(ti);
                    if(t->type == TOKTYPE_DO) {
                        nexttok(ti);
                        p_statementlist(ti);
                        t = tok(ti);
                        if(t->type == TOKTYPE_ENDFOR) {
                            nexttok(ti);
                        }
                        else {
                            //syntax error
                            adderr(ti, "Syntax Error", t->lex, t->line, "endfor", NULL);
                            synerr_rec(ti);
                        }
                    }
                    else {
                        //syntax error
                        adderr(ti, "Syntax Error", t->lex, t->line, "do", NULL);
                        synerr_rec(ti);
                    }
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "<-", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_SWITCH:
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENPAREN) {
                nexttok(ti);
                p_expression(ti);
                t = tok(ti);
                if(t->type == TOKTYPE_CLOSEPAREN) {
                    nexttok(ti);
                    p_caselist(ti);
                    t = tok(ti);
                    if(t->type == TOKTYPE_ENDSWITCH) {
                        nexttok(ti);
                    }
                    else {
                        //syntax error
                        adderr(ti, "Syntax Error", t->lex, t->line, "endswitch", NULL);
                        synerr_rec(ti);
                    }
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "(", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_LISENER:
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENPAREN) {
                nexttok(ti);
                p_arglist(ti);
                t = tok(ti);
                if(t->type == TOKTYPE_CLOSEPAREN) {
                    t = nexttok(ti);
                    if(t->type == TOKTYPE_OPENBRACE) {
                        nexttok(ti);
                        p_statementlist(ti);
                        t =tok(ti);
                        if(t->type == TOKTYPE_CLOSEBRACE) {
                            nexttok(ti);
                        }
                        else {
                            //syntax error
                            adderr(ti, "Syntax Error", t->lex, t->line, "}", NULL);
                            synerr_rec(ti);
                        }
                    }
                    else {
                        //syntax error
                        adderr(ti, "Syntax Error", t->lex, t->line, "{", NULL);
                        synerr_rec(ti);
                    }
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "(", NULL);
                synerr_rec(ti);
            }
            break;
        default:
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "if", "while", "for", "switch", NULL);
            synerr_rec(ti);
            break;
    }
}

void p_caselist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_CASE) {
        nexttok(ti);
        p_arglist(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_MAP) {
            nexttok(ti);
            p_expression(ti);
            p_caselist(ti);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "->", NULL);
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_DEFAULT) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_MAP) {
            nexttok(ti);
            p_expression(ti);
            p_caselist(ti);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "->", NULL);
            synerr_rec(ti);
        }
    }
    else {
        //epsilon production
    }
}

void p_elseif(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ELIF) {
        nexttok(ti);
        p_expression(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_THEN) {
            nexttok(ti);
            p_statementlist(ti);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "then", NULL);
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_ELSE) {
        nexttok(ti);
        p_statementlist(ti);
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "else", "elif", NULL);
        synerr_rec(ti);
    }
}

void p_dec(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_VAR) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            p_opttype(ti);
            p_assign(ti);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
            synerr_rec(ti);
        }
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "var", NULL);
        synerr_rec(ti);
    }
}

void p_opttype(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COLON) {
        nexttok(ti);
        p_type(ti);
    }
    else {
        //epsilon production
    }
}


void p_type(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_VOID:
            nexttok(ti);
            break;
        case TOKTYPE_INTEGER:
            nexttok(ti);
            p_array(ti);
            break;
        case TOKTYPE_REAL:
            nexttok(ti);
            p_array(ti);
            break;
        case TOKTYPE_STRINGTYPE:
            nexttok(ti);
            p_array(ti);
            break;
        case TOKTYPE_REGEXTYPE:
            nexttok(ti);
            p_array(ti);
            break;
        case TOKTYPE_SET:
            nexttok(ti);
            p_array(ti);
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            p_optparamlist(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
                p_array(ti);
                t = tok(ti);
                if(t->type == TOKTYPE_MAP) {
                    nexttok(ti);
                    p_type(ti);
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "->", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
                synerr_rec(ti);
            }
            break;
        default:
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "void", "int", "real", "string", "regex", "set", "(", NULL);
            synerr_rec(ti);
            break;
    }

}

void p_array(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_OPENBRACKET) {
        nexttok(ti);
        p_expression(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACKET) {
            nexttok(ti);
            p_array(ti);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "]", NULL);
            synerr_rec(ti);
        }
    }
    else {
        //epsilon production
    }
}

void p_optparamlist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_VAR) {
        p_paramlist(ti);
    }
    else {
        //epsilon production
    }
}

void p_paramlist(tokiter_s *ti)
{
    p_dec(ti);
    p_paramlist_(ti);
}

void p_paramlist_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        p_dec(ti);
        p_paramlist_(ti);
    }
    else {
        //epsilon production
    }
}

void p_set(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        p_expression(ti);
        p_optnext(ti);
        p_set_(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACE) {
            nexttok(ti);
            
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "}", NULL);
            synerr_rec(ti);
        }
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "{", NULL);
        synerr_rec(ti);
    }
}

void p_optnext(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_MAP) {
        nexttok(ti);
        p_expression(ti);
    }
    else if(t->type == TOKTYPE_RANGE) {
        nexttok(ti);
        p_expression(ti);
    }
    else {
        //epsilon production
    }
}

void p_set_(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        p_expression(ti);
        p_optnext(ti);
        p_set_(ti);
    }
    else {
        //epsilon production
    }
}

void synerr_rec(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type != TOKTYPE_EOF)
        nexttok(ti);
}

void printerrs(errlist_s *err)
{
    if(err) {
        while(err) {
            puts(err->msg);
            err = err->next;
        }
    }
    else {
        puts("no errors");
    }
}

void adderr(tokiter_s *ti, char *prefix, char *got, uint16_t line, ...)
{
    va_list argp;
    uint8_t i = 0;
    errlist_s *e;
    size_t totallen = strlen(prefix), nargs, curr;
    char *args[MAX_ERRARGS], *s;
    
    va_start(argp, line);
    
    while((s = va_arg(argp, char *))) {
        totallen += strlen(s);
        args[i++] = s;
    }
    
    nargs = i;
    
    if(nargs > 1) {
        totallen += (sizeof(" at line: ") - 1) + ndigits(line) + (sizeof(". Expected ") - 1) +
        (nargs - 1)*2 + sizeof("or ") + sizeof(" but got ") + strlen(got);
        e = alloc(sizeof(*e) + totallen);
        e->next = NULL;
        
        curr = sprintf(e->msg, "%s at line: %u. Expected ", prefix , line);
        
        for(i = 0; i < nargs - 1; i++)
            curr += sprintf(&e->msg[curr], "%s, ", args[i]);
        
        sprintf(&e->msg[curr], "or %s but got %s.", args[i], got);
    }
    else {
        totallen += (sizeof(" at line: ") - 1) + ndigits(line) + (sizeof(". Expected ") - 1) +
        sizeof(" but got ") + strlen(got) + 1;
        e = alloc(sizeof(*e) + totallen);
        e->next = NULL;
        
        curr = sprintf(e->msg, "%s at line: %u. Expected %s but got %s.", prefix , line, args[0], got);
    }
    
    va_end(argp);
    
    if(ti->err)
        ti->ecurr->next = e;
    else
        ti->err = e;
    ti->ecurr = e;
}
