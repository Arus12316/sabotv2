/*
 <statementlist> -> <statement> <statementlist> | ε
 
 <statement> ->  <expression> ; | <control> | <dec> ; | return <expression>
 
 <expression> -> <simple_expression> <expression'>
 
 <expression'> -> relop <simple_expression> | ε
 
 <simple_expression> -> <sign> <term> <simple_expression'>
                        |
                        <term> <simple_expression'>
 
 <simple_expression'> -> addop <term> <simple_expression'> | ε
 
 <term> -> <factor> <term'>
 
 <term'> -> mulop <factor> <term'> | ε
 
 <factor> -> <factor'> <optexp>
 
 <optexp> -> ^ <factor> | ε
 
 <factor'> ->   id <factor''> <assign> { <factor''>.ident := id }
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
 
 
 <factor''> -> [ <expression> ] <factor''> | . id <factor''> | ( <optarglist> ) <factor''> | ε
 
 <assign> -> assignop <expression> | ε
 
 <lambda> -> @ (<optparamlist>) openbrace <statementlist> closebrace
 
 <sign> -> + | -
 
 <optarglist> -> <arglist> | ε
 <arglist> -> <expression> <arglist'>
 <arglist'> -> , <expression> <arglist'> | ε
 
 <control> ->   <if>
                |
                while <expression> openbrace <statementlist> closebrace
                |
                for id <- <expression> openbrace <statementlist> closebrace
                |
                <switch>
                |
                listener(<arglist>) openbrace <statementlist> closebrace
 
 <if> -> if <expression> openbrace <statementlist> closebrace <elseif>
 
 <switch> -> switch(<expression>) openbrace <caselist> closebrace
 
 <caselist> ->
                case <arglist> map <expression> <caselist>
                |
                default map <expression> <caselist>
                |
                ε
 
 <elseif> -> elif <expression> openbrace <statementlist> closebrace <elseif> | else openbrace <statementlist> closebrace
 
 <dec> -> var id <opttype> <assign> | class id <inh> { <declist> }
 
 <opttype> -> : <type> | ε
 
 <type> -> void | integer <array> | real <array> | String <array> | Regex <array> | set <array> | id <array> |( <optparamlist> ) <array> map <type>
 
 <inh> -> : id | ε
 <declist> -> <dec> ; <declist> | ε

 
 <array> -> [ <expression> ] <array> | ε
 
 <optparamlist> -> <paramlist> | ε
 <param> -> id <opttype> <assign>
 <paramlist> -> <param> <paramlist'>
 <paramlist'> -> , <param> <paramlist'> | ε
 
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
#include <assert.h>

#define TOKCHUNK_SIZE 16
#define MAX_ERRARGS 256
#define PUSHSCOPE() pushscope(ti)
#define POPSCOPE() popscope(ti)

#define SYM_TABLE_SIZE 19

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
    TOKTYPE_ELIF,
    TOKTYPE_ELSE,
    TOKTYPE_SWITCH,
    TOKTYPE_CASE,
    TOKTYPE_DEFAULT,
    TOKTYPE_WHILE,
    TOKTYPE_FOR,
    TOKTYPE_LISENER,
    TOKTYPE_VAR,
    TOKTYPE_VOID,
    TOKTYPE_INTEGER,
    TOKTYPE_REAL,
    TOKTYPE_STRINGTYPE,
    TOKTYPE_REGEXTYPE,
    TOKTYPE_LIST,
    TOKTYPE_RETURN,
    TOKTYPE_CLASS,
    TOKTYPE_UNNEG
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

enum {
    TYPE_VOID,
    TYPE_NUM,
    TYPE_INTEGER,
    TYPE_REAL,
    TYPE_STRING,
    TYPE_REGEX,
    TYPE_SET,
    TYPE_LIST,
    TYPE_CLOSURE,
    TYPE_TYPEEXP,
    TYPE_EPSILON,
    TYPE_OP,
    TYPE_PARAMLIST,
    TYPE_DEC,
    TYPE_IDENT,
    TYPE_NODE,
    TYPE_INCOMPLETE
};

typedef struct tok_s tok_s;
typedef struct tokchunk_s tokchunk_s;
typedef struct tokiter_s tokiter_s;
typedef struct node_s node_s;

typedef struct rec_s rec_s;
typedef struct scope_s scope_s;

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
    scope_s *scope;
};

static struct keyw_s {
    char *str;
    uint8_t type;
}
keywords[] = {
    {"not", TOKTYPE_NOT},
    {"if", TOKTYPE_IF},
    {"elif", TOKTYPE_ELIF},
    {"else", TOKTYPE_ELSE},
    {"while", TOKTYPE_WHILE},
    {"for", TOKTYPE_FOR},
    {"switch", TOKTYPE_SWITCH},
    {"case", TOKTYPE_CASE},
    {"default", TOKTYPE_DEFAULT},
    {"listener", TOKTYPE_LISENER},
    {"var", TOKTYPE_VAR},
    {"void", TOKTYPE_VOID},
    {"int", TOKTYPE_INTEGER},
    {"real", TOKTYPE_REAL},
    {"string", TOKTYPE_STRINGTYPE},
    {"regex", TOKTYPE_REGEXTYPE},
    {"list", TOKTYPE_LIST},
    {"return", TOKTYPE_RETURN},
    {"class", TOKTYPE_CLASS}
};

struct node_s
{
    uint8_t type;
    
    tok_s *tok;
    scope_s *scope;
    
    unsigned nchildren;
    node_s **children;
    node_s *parent;
};

struct rec_s
{
    char *key;
    rec_s *next;
};

struct scope_s
{
    rec_s *table[SYM_TABLE_SIZE];
    scope_s *parent;
    unsigned nchildren;
    scope_s **children;
};

static tokiter_s *lex(char *src);
static tok_s *mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att);
static bool trykeyword(tokchunk_s **list, tok_s **prev, uint16_t line, char *str);
static void printtoks(tokchunk_s *list);

static tok_s *tok(tokiter_s *ti);
static tok_s *nexttok(tokiter_s *ti);

static void start(tokiter_s *ti);
static void p_statementlist(tokiter_s *ti);
static void p_statement(tokiter_s *ti);
static node_s *p_expression(tokiter_s *ti);
static void p_expression_(tokiter_s *ti, node_s *exp);
static node_s *p_simple_expression(tokiter_s *ti);
static void p_simple_expression_(tokiter_s *ti, node_s *sexp);
static node_s *p_term(tokiter_s *ti);
static void p_term_(tokiter_s *ti, node_s *term);
static node_s *p_factor(tokiter_s *ti);
static void p_optexp(tokiter_s *ti, node_s *factor);
static node_s *p_factor_(tokiter_s *ti);
static void p_factor__(tokiter_s *ti, node_s *root);
static void p_assign(tokiter_s *ti, node_s *root);
static node_s *p_lambda(tokiter_s *ti);
static int p_sign(tokiter_s *ti);
static node_s *p_optarglist(tokiter_s *ti);
static node_s *p_arglist(tokiter_s *ti);
static void p_arglist_(tokiter_s *ti, node_s *root);
static void p_control(tokiter_s *ti);
static void p_if(tokiter_s *ti);
static void p_switch(tokiter_s *ti);
static void p_caselist(tokiter_s *ti);
static void p_elseif(tokiter_s *ti);
static node_s *p_dec(tokiter_s *ti);
static node_s *p_opttype(tokiter_s *ti);
static node_s *p_type(tokiter_s *ti);
static void p_inh(tokiter_s *ti);
static void p_declist(tokiter_s *ti);
static node_s *p_array(tokiter_s *ti);
static node_s *p_optparamlist(tokiter_s *ti);
static node_s *p_paramlist(tokiter_s *ti);
static void p_paramlist_(tokiter_s *ti, node_s *root);
static node_s *p_set(tokiter_s *ti);
static void p_optnext(tokiter_s *ti);
static void p_set_(tokiter_s *ti, node_s *set);
static void synerr_rec(tokiter_s *ti);

static node_s *node_s_(void);
static void addchild(node_s *root, node_s *c);

static unsigned pjwhash(char *key);
static bool addident(scope_s *root, node_s *ident);
static scope_s *idtinit(scope_s *parent);
static scope_s *pushscope(tokiter_s *ti);
static inline void popscope(tokiter_s *ti);

static void emit(char *, ...);

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
    tok_s *prev = NULL;
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
            case '#':
                do {
                    src++;
                }
                while(*src && *src != '\n');
                break;
            case '\n':
                line++;
            case ' ':
            case '\t':
            case '\v':
            case '\r':
                src++;
                break;
            case ',':
                prev = mtok(&curr, line, ",", 1, TOKTYPE_COMMA, TOKATT_DEFAULT);
                src++;
                break;
            case '.':
                prev = mtok(&curr, line, ".", 1, TOKTYPE_DOT, TOKATT_DEFAULT);
                src++;
                break;
            case '(':
                prev = mtok(&curr, line, "(", 1, TOKTYPE_OPENPAREN, TOKATT_DEFAULT);
                src++;
                break;
            case ')':
                prev = mtok(&curr, line, ")", 1, TOKTYPE_CLOSEPAREN, TOKATT_DEFAULT);
                src++;
                break;
            case '[':
                prev = mtok(&curr, line, "[", 1, TOKTYPE_OPENBRACKET, TOKATT_DEFAULT);
                src++;
                break;
            case ']':
                prev = mtok(&curr, line, "]", 1, TOKTYPE_CLOSEBRACKET, TOKATT_DEFAULT);
                src++;
                break;
            case '{':
                prev = mtok(&curr, line, "{", 1, TOKTYPE_OPENBRACE, TOKATT_DEFAULT);
                src++;
                break;
            case '}':
                prev = mtok(&curr, line, "}", 1, TOKTYPE_CLOSEBRACE, TOKATT_DEFAULT);
                src++;
                break;
            case '@':
                prev = mtok(&curr, line, "@", 1, TOKTYPE_LAMBDA, TOKATT_DEFAULT);
                src++;
                break;
            case '!':
                prev = mtok(&curr, line, "!", 1, TOKTYPE_NOT, TOKATT_DEFAULT);
                src++;
                break;
            case '&':
                prev = mtok(&curr, line, "!", 1, TOKTYPE_MULOP, TOKATT_AND);
                src++;
                break;
            case '|':
                prev = mtok(&curr, line, "!", 1, TOKTYPE_ADDOP, TOKATT_OR);
                src++;
                break;
            case '<':
                if(*(src + 1) == '-') {
                    prev = mtok(&curr, line, "<-", 2, TOKTYPE_FORVAR, TOKATT_DEFAULT);
                    src += 2;
                }
                else if(*(src + 1) == '>') {
                    prev = mtok(&curr, line, "<>", 2, TOKTYPE_RELOP, TOKATT_NEQ);
                    src += 2;
                }
                else if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "<=", 2, TOKTYPE_RELOP, TOKATT_LEQ);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "<", 1, TOKTYPE_RELOP, TOKATT_L);
                    src++;
                }
                break;
            case '>':
                if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, ">=", 2, TOKTYPE_RELOP, TOKATT_GEQ);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, ">", 1, TOKTYPE_RELOP, TOKATT_G);
                    src++;
                }
                break;
            case '+':
                if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "+=", 2, TOKTYPE_ASSIGN, TOKATT_ADD);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "+", 1, TOKTYPE_ADDOP, TOKATT_ADD);
                    src++;
                }
                break;
            case '-':
                if(*(src + 1) == '>') {
                    prev = mtok(&curr, line, "->", 2, TOKTYPE_MAP, TOKATT_DEFAULT);
                    src += 2;
                }
                else if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "-=", 2, TOKTYPE_ASSIGN, TOKATT_SUB);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "-", 1, TOKTYPE_ADDOP, TOKATT_SUB);
                    src++;
                }
                break;
            case '*':
                if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "*=", 2, TOKTYPE_ASSIGN, TOKATT_MULT);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "*", 1, TOKTYPE_MULOP, TOKATT_MULT);
                    src++;
                }
                break;
            case '/':
                if(prev) {
                    switch(prev->type) {
                        case TOKTYPE_NUM:
                        case TOKTYPE_STRING:
                        case TOKTYPE_REGEX:
                        case TOKTYPE_CLOSEPAREN:
                        case TOKTYPE_CLOSEBRACKET:
                        case TOKTYPE_CLOSEBRACE:
                            if(*(src + 1) == '=') {
                                prev = mtok(&curr, line, "/=", 2, TOKTYPE_ASSIGN, TOKATT_DIV);
                                src += 2;
                            }
                            else {
                                prev = mtok(&curr, line, "/", 1, TOKTYPE_MULOP, TOKATT_DIV);
                                src++;
                            }
                            break;
                        default:
                            goto parse_regex;
                    }
                }
                else {
                parse_regex:
                    bptr = src++;
                    while(*src != '/') {
                        if(*src)
                            src++;
                        else {
                            adderr(ti, "Lexical Error", "EOF", line, "/", NULL);
                            break;
                        }
                    }
                    c = *++src;
                    *src = '\0';
                    prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_REGEX, TOKATT_DEFAULT);
                    *src = c;
                }
                break;
            case '%':
                if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "%=", 2, TOKTYPE_ASSIGN, TOKATT_MOD);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "%", 1, TOKTYPE_MULOP, TOKATT_MOD);
                    src++;
                }
                break;
            case ':':
                if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, ":=", 2, TOKTYPE_ASSIGN, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, ":", 1, TOKTYPE_COLON, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case ';':
                prev = mtok(&curr, line, ";", 1, TOKTYPE_SEMICOLON, TOKATT_DEFAULT);
                src++;
                break;
            case '^':
                if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "^=", 2, TOKTYPE_ASSIGN, TOKATT_EXP);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "^", 1, TOKTYPE_EXPOP, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case '=':
                if(*(src + 1) == '>') {
                    prev = mtok(&curr, line, "=>", 2, TOKTYPE_RANGE, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "=", 1, TOKTYPE_RELOP, TOKATT_DEFAULT);
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
                prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_STRING, TOKATT_DEFAULT);
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
                                adderr(ti, "Lexical Error", ".", line, "valid number", NULL);
                                src++;
                            }
                        }
                        else {
                            c = *src;
                            *src = '\0';
                            
                            if(gotdot)
                                prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMREAL);
                            else
                                prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_NUM, TOKATT_NUMINT);
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
                    if(!trykeyword(&curr, &prev, line, bptr))
                        prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_IDENT, TOKATT_DEFAULT);
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

tok_s *mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att)
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
    
    return &l->tok[size];;
}

bool trykeyword(tokchunk_s **list, tok_s **prev, uint16_t line, char *str)
{
    int i;
    size_t len;
    
    for(i = 0; i < sizeof(keywords) / sizeof(struct keyw_s); i++) {
        if(!strcmp(str, keywords[i].str)) {
            len = strlen(keywords[i].str);
            *prev = mtok(list, line, keywords[i].str, len, keywords[i].type, TOKATT_DEFAULT);
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
    
    ti->scope = idtinit(NULL);
    
    p_statementlist(ti);
    t = tok(ti);
    
    if(t->type != TOKTYPE_EOF) {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "EOF", NULL);
    }
}

void p_statementlist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_CLASS:
        case TOKTYPE_VAR:
        case TOKTYPE_LISENER:
        case TOKTYPE_SWITCH:
        case TOKTYPE_FOR:
        case TOKTYPE_WHILE:
        case TOKTYPE_IF:
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
            p_statementlist(ti);
            break;
        case TOKTYPE_CLOSEBRACE:
        case TOKTYPE_EOF:
            //epsilon production
            break;
        default:
            adderr(ti, "Syntax Error", t->lex, t->line, "class", "var", "listener", "switch", "for", "while", "if",
                                                        "+", "-", "{", "@", "regex", "string", "!", "(", "number", "identifier", "return", NULL);
            synerr_rec(ti);
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
        case TOKTYPE_CLASS:
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

node_s *p_expression(tokiter_s *ti)
{
    node_s *node, *exp;
    
    exp = node_s_();
    exp->type = TYPE_NODE;

    node = p_simple_expression(ti);
    addchild(exp, node);
    p_expression_(ti, exp);
    
    return node;
}

void p_expression_(tokiter_s *ti, node_s *exp)
{
    node_s *s = NULL, *op = NULL;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_RELOP) {
        nexttok(ti);
        s = p_simple_expression(ti);
        
        op = node_s_();
        op->type = TYPE_OP;
        
        addchild(exp, op);
        addchild(exp, s);
    }
}

node_s *p_simple_expression(tokiter_s *ti)
{
    int sign = 0;
    node_s *n, *u = NULL, *sexp;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ADDOP)
        sign = p_sign(ti);
    n = p_term(ti);
    
    if(sign == -1) {
        u = node_s_();
        u->type = TYPE_OP;
        u->tok = t;
        t->type = TOKTYPE_UNNEG;
        addchild(u, n);
        n = u;
    }
    
    sexp = node_s_();
    sexp->type = TYPE_OP;
    
    addchild(sexp, n);
    
    p_simple_expression_(ti, sexp);
    return n;
}

void p_simple_expression_(tokiter_s *ti, node_s *sexp)
{
    node_s *term = NULL, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ADDOP) {
        nexttok(ti);
        term = p_term(ti);
        
        op = node_s_();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(sexp, op);
        addchild(sexp, term);
        
        p_simple_expression_(ti, sexp);
    }
}

node_s *p_term(tokiter_s *ti)
{
    node_s *f, *term;
    
    term = node_s_();
    term->type = TYPE_NODE;
    
    f = p_factor(ti);
    addchild(term, f);
    
    p_term_(ti, term);
    
    return term;
}

void p_term_(tokiter_s *ti, node_s *term)
{
    node_s *f = NULL, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_MULOP) {
        nexttok(ti);
        f = p_factor(ti);
        
        op = node_s_();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(term, op);
        addchild(term, f);
        p_term_(ti, term);
    }
    
}

node_s *p_factor(tokiter_s *ti)
{
    node_s *f, *fact;
    
    fact = node_s_();
    fact->type = TYPE_NODE;
    
    f = p_factor_(ti);
    addchild(fact, f);
    p_optexp(ti, fact);
    
    return fact;
}

void p_optexp(tokiter_s *ti, node_s *factor)
{
    tok_s *t = tok(ti);
    node_s *f = NULL, *op;
    
    if(t->type == TOKTYPE_EXPOP) {
        nexttok(ti);
        f = p_factor(ti);
        
        op = node_s_();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(factor, op);
        addchild(factor, f);
    }
}

node_s *p_factor_(tokiter_s *ti)
{
    node_s *n, *ident;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
            nexttok(ti);
            
            n = node_s_();
            n->type = TYPE_NODE;
            
            ident = node_s_();
            ident->type = TYPE_IDENT;
            ident->tok = t;
            
            addchild(n, ident);

            p_factor__(ti, n);
            p_assign(ti, n);
            break;
        case TOKTYPE_NUM:
            nexttok(ti);
            n = node_s_();
            n->type = TYPE_NUM;
            n->tok = t;
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            n = p_expression(ti);
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
            n = node_s_();
            n->type = TYPE_STRING;
            n->tok = t;
            nexttok(ti);
            break;
        case TOKTYPE_REGEX:
            n = node_s_();
            n->type = TYPE_REGEX;
            n->tok = t;
            nexttok(ti);
            break;
        case TOKTYPE_OPENBRACE:
            n = p_set(ti);
            break;
        case TOKTYPE_LAMBDA:
            n = p_lambda(ti);
            n->tok = t;
            break;
        default:
            //syntax error
            n = NULL;
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", "number", "(", "string", "regex", "{", "@", NULL);
            synerr_rec(ti);
            break;
    }
    return n;
}

void p_factor__(tokiter_s *ti, node_s *root)
{
    node_s *op, *exp, *ident, *opt;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_OPENBRACKET:
            nexttok(ti);
            exp = p_expression(ti);
            
            op = node_s_();
            op->type = TYPE_OP;
            op->tok = t;
        
            addchild(root, op);
            addchild(root, exp);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACKET) {
                nexttok(ti);
                p_factor__(ti, root);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "]", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_DOT:
            op = node_s_();
            op->type = TYPE_OP;
            op->tok = t;

            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                nexttok(ti);
                ident = node_s_();
                ident->type = TYPE_IDENT;
                addchild(root, op);
                addchild(root, ident);
                p_factor__(ti, root);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            
            op = node_s_();
            op->type = TYPE_OP;
            op->tok = t;
            
            opt = p_optarglist(ti);
            addchild(root, op);
            addchild(root, opt);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
                p_factor__(ti, root);
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

void p_assign(tokiter_s *ti, node_s *root)
{
    node_s *exp, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ASSIGN) {
        nexttok(ti);
        exp = p_expression(ti);
        
        op = node_s_();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(root, exp);
    }
}

node_s *p_lambda(tokiter_s *ti)
{
    node_s *lambda;
    //so far, a check that token is '@' is redundant
    tok_s *t = nexttok(ti);
    
    lambda = node_s_();
    lambda->type = TYPE_CLOSURE;
    
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
    return lambda;
}

int p_sign(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->att == TOKATT_SUB) {
        nexttok(ti);
        return -1;
    }
    else if(t->att == TOKATT_ADD) {
        nexttok(ti);
        return 1;
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "+", "-", NULL);
        synerr_rec(ti);
        return 1;
    }
}

node_s *p_optarglist(tokiter_s *ti)
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
            return p_arglist(ti);
            break;
        default:
            //epsilon production
            return NULL;
    }
}

node_s *p_arglist(tokiter_s *ti)
{
    node_s *root = node_s_(), *n;
    
    n = p_expression(ti);
    addchild(root, n);
    p_arglist_(ti, root);

    return root;
}

void p_arglist_(tokiter_s *ti, node_s *root)
{
    node_s *n;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        n = p_expression(ti);
        addchild(root, n);
        
        p_arglist_(ti, root);
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
            p_if(ti);
            break;
        case TOKTYPE_WHILE:
            nexttok(ti);
            p_expression(ti);
            t = tok(ti);
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
            break;
        case TOKTYPE_FOR:
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                t = nexttok(ti);
                if(t->type == TOKTYPE_FORVAR) {
                    nexttok(ti);
                    p_expression(ti);
                    t = tok(ti);
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
            p_switch(ti);
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

void p_if(tokiter_s *ti)
{
    tok_s *t = nexttok(ti);
    p_expression(ti);
    t = tok(ti);
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        p_statementlist(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACE) {
            nexttok(ti);
            p_elseif(ti);
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

void p_switch(tokiter_s *ti)
{
    tok_s *t = nexttok(ti);
    
    if(t->type == TOKTYPE_OPENPAREN) {
        nexttok(ti);
        p_expression(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEPAREN) {
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                p_caselist(ti);
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
        if(t->type == TOKTYPE_OPENBRACE) {
            nexttok(ti);
            p_statementlist(ti);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACE) {
                nexttok(ti);
                p_elseif(ti);
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
    else if(t->type == TOKTYPE_ELSE) {
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
        adderr(ti, "Syntax Error", t->lex, t->line, "else", "elif", NULL);
        synerr_rec(ti);
    }
}

node_s *p_dec(tokiter_s *ti)
{
    node_s *ident, *dectype = NULL, *dec = NULL;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_VAR) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            
            dec = node_s_();
            dec->type = TYPE_DEC;
            
            ident = node_s_();
            ident->type = TYPE_IDENT;
            ident->tok = t;
            addchild(dec, ident);
            
            dectype = p_opttype(ti);
            addchild(dec, dectype);
            p_assign(ti, dec);
            
            if(addident(ti->scope, ident)) {
                adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
            }
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_CLASS) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            t = nexttok(ti);
            p_inh(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                p_declist(ti);
                t = tok(ti);
                if(t->type == TOKTYPE_CLOSEBRACE) {
                    nexttok(ti);
                }
                else {
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
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
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
            synerr_rec(ti);
        }
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "var", NULL);
        synerr_rec(ti);
    }
    return dec;
}

node_s *p_opttype(tokiter_s *ti)
{
    node_s *type = NULL;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COLON) {
        nexttok(ti);
        type = p_type(ti);
    }
    else {
        //epsilon production
        type = NULL;
    }
    return type;
}


node_s *p_type(tokiter_s *ti)
{
    node_s *type, *array, *opt;
    tok_s *t = tok(ti);
    
    type = node_s_();
    type->type = TYPE_TYPEEXP;
    
    switch(t->type) {
        case TOKTYPE_VOID:
            nexttok(ti);
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok= t;
            break;
        case TOKTYPE_INTEGER:
            nexttok(ti);
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            array = p_array(ti);
            addchild(type, array);
            break;
        case TOKTYPE_REAL:
            nexttok(ti);
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            array = p_array(ti);
            addchild(type, array);
            break;
        case TOKTYPE_STRINGTYPE:
            nexttok(ti);
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            array = p_array(ti);
            addchild(type, array);
            break;
        case TOKTYPE_REGEXTYPE:
            nexttok(ti);
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            array = p_array(ti);
            addchild(type, array);
            break;
        case TOKTYPE_LIST:
            nexttok(ti);
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            array = p_array(ti);
            addchild(type, array);
            break;
        case TOKTYPE_IDENT:
            nexttok(ti);
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            array = p_array(ti);
            addchild(type, array);
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            
            type = node_s_();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            
            opt = p_optparamlist(ti);
            
            addchild(type, opt);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
                array = p_array(ti);
                addchild(type, array);
                t = tok(ti);
                if(t->type == TOKTYPE_MAP) {
                    nexttok(ti);
                    p_type(ti);
                }
                else {
                    type = NULL;
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "->", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                type = NULL;
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
                synerr_rec(ti);
            }
            break;
        default:
            type = NULL;
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "void", "int", "real", "string", "regex", "set", "(", NULL);
            synerr_rec(ti);
            break;
    }
    return type;
}

void p_inh(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COLON) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
        }
        else {
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
            synerr_rec(ti);
        }
    }
}

void p_declist(tokiter_s *ti)
{
    tok_s *t;
    
    for(t = tok(ti); t->type == TOKTYPE_VAR || t->type == TOKTYPE_CLASS; t = nexttok(ti)) {
        p_dec(ti);
        t = tok(ti);
        if(t->type != TOKTYPE_SEMICOLON) {
            adderr(ti, "Syntax Error", t->lex, t->line, ";", NULL);
            synerr_rec(ti);
        }
    }
}

node_s *p_array(tokiter_s *ti)
{
    node_s *root = node_s_(), *n;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_OPENBRACKET) {
        nexttok(ti);
        n = p_expression(ti);
        addchild(root, n);
        
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACKET) {
            nexttok(ti);
            return p_array(ti);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "]", NULL);
            synerr_rec(ti);
            return root;
        }
    }
    else {
        //epsilon production
        return root;
    }
}

node_s *p_param(tokiter_s *ti)
{
    node_s *n, *ident, *opt;
    tok_s *t = tok(ti);
    
    
    if(t->type == TOKTYPE_IDENT) {
        nexttok(ti);
        n = node_s_();
        n->type = TYPE_NODE;

        ident = node_s_();
        ident->type = TYPE_IDENT;
        ident->tok = t;
        
        if(addident(ti->scope, ident)) {
            adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
        }
        opt = p_opttype(ti);
        addchild(n, ident);
        addchild(n, opt);
        p_assign(ti, n);
        return n;
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
        synerr_rec(ti);
        return NULL;
    }
}

node_s *p_optparamlist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        return p_paramlist(ti);
    }
    else {
        //epsilon production
        return NULL;
    }
}

node_s *p_paramlist(tokiter_s *ti)
{
    node_s *root = node_s_(), *dec;

    root->type = TYPE_PARAMLIST;
    root->scope = PUSHSCOPE();
    dec = p_param(ti);
    
    addchild(root, dec);
    p_paramlist_(ti, root);
    
    POPSCOPE();
    
    return root;
}

void p_paramlist_(tokiter_s *ti, node_s *root)
{
    node_s *dec;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        
        dec = p_param(ti);
        
        addchild(root, dec);
        p_paramlist_(ti, root);
    }
    else {
        //epsilon production
    }
}

node_s *p_set(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    node_s *set = node_s_(), *n;
    
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        n = p_expression(ti);
        p_optnext(ti);
        addchild(set, n);
        p_set_(ti, set);
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
    return set;
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


void p_set_(tokiter_s *ti, node_s *set)
{
    tok_s *t = tok(ti);
    node_s *n;
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        n = p_expression(ti);
        p_optnext(ti);
        addchild(set, n);
        p_set_(ti, set);
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

node_s *node_s_(void)
{
    node_s *e = alloc(sizeof *e);
    
    e->nchildren = 0;
    return e;
}

void addchild(node_s *root, node_s *c)
{
    if(c) {
        if(root->nchildren)
            root->children = ralloc(root->children, (root->nchildren + 1) * sizeof(*root->children));
        else
            root->children = alloc(sizeof *root->children);
        root->children[root->nchildren] = c;
        root->nchildren++;
    }
}

/*
 pjw hash function 
 
 credit to:
 -Alfred V. Aho, Ravi Sethi, and Jeffrey D. Ullman, 1986.
 */
unsigned pjwhash(char *key)
{
    unsigned h = 0, g;
    
    while(*key) {
        h = (h << 4) + *key++;
        if((g = h & (0xfu << (sizeof(unsigned)*8-4))) != 0)
            h = (h ^ (g >> 24)) ^ g;
    }
    return h % SYM_TABLE_SIZE;
}

bool addident(scope_s *root, node_s *ident)
{
    char *key = ident->tok->lex;
    unsigned index = pjwhash(key);
    rec_s **ptr = &root->table[index], *i = *ptr, *n;
    
    if(i) {
        while(i->next) {
            if(!strcmp(i->key, key))
                return true;
            i = i->next;
        }
        if(!strcmp(i->key, key))
            return true;
        ptr = &i->next;
    }
    n = alloc(sizeof *n);
    n->key = key;
    n->next = NULL;
    *ptr = n;
    return false;
}

scope_s *idtinit(scope_s *parent)
{
    scope_s *s = allocz(sizeof *s);
    
    s->parent = parent;
    return s;
}

scope_s *pushscope(tokiter_s *ti)
{
    scope_s *o = ti->scope, *n = idtinit(o);
    
    if(o->children)
        o->children = ralloc(o->children, (o->nchildren + 1) * sizeof(*o->children));
    else
        o->children = alloc(sizeof *o->children);
    o->children[o->nchildren] = n;
    o->nchildren++;
    ti->scope = n;
    return n;
}

inline void popscope(tokiter_s *ti)
{
    ti->scope = ti->scope->parent;
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
