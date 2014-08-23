/*
 <statementlist> -> <statement> <statementlist> | ε
 
 <statement> ->  <expression> | <control> | <dec> | return <expression> | ; | continue | break
 
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
                |
                <if>
                |
                <switch>
 
 
 <factor''> -> [ <expression> ] <factor''> | . id <factor''> | ( <optarglist> ) <factor''> | ε
 
 <assign> -> assignop <expression> | ε
 
 <lambda> -> @ (<optparamlist>) openbrace <statementlist> closebrace
 
 <sign> -> + | -
 
 <optarglist> -> <arglist> | ε
 <arglist> -> <expression> <arglist'>
 <arglist'> -> , <expression> <arglist'> | ε
 
 <control> ->	while <expression> <controlsuffix>
                |
                for id <- <expression> <controlsuffix>
                |
                listener(<arglist>) <controlsuffix>
                |
                do <controlsuffix> while <expression>
 
 
 <if> -> if <expression> <controlsuffix> <else>
 
    {
    }
 
 <switch> -> switch(<expression>) openbrace <caselist> closebrace
 
 <caselist> ->  case <arglist> map <expression> <caselist>
                |
                default map <expression> <caselist>
                |
                ε
 
 <else> -> else openbrace <statementlist> closebrace | ε
 
 <controlsuffix> -> openbrace <statementlist> closebrace | <statement>
 
 <dec> -> var id <opttype> <assign> | class id <inh> { <declist> }
 
 <opttype> -> : <type> | ε
 
 <type> -> void | integer <array> | real <array> | String <array> | Regex <array> | set <array> | id <array> |( <optparamlist> ) <array> map <type>
 
 <inh> -> : id | ε
 <declist> -> <dec> <optsemicolon> <declist> | ε

 <optsemicolon> -> ; | ε
 
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
#define MAKENODE() node_s_(ti->scope)

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
    TOKTYPE_ELSE,
    TOKTYPE_SWITCH,
    TOKTYPE_CASE,
    TOKTYPE_DEFAULT,
    TOKTYPE_DO,
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
    TOKTYPE_BREAK,
    TOKTYPE_CONTINUE,
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
typedef struct flow_s flow_s;
typedef struct flnode_s flnode_s;

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
    node_s *graph;
};

static struct keyw_s {
    char *str;
    uint8_t type;
}
keywords[] = {
    {"not", TOKTYPE_NOT},
    {"if", TOKTYPE_IF},
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
    {"class", TOKTYPE_CLASS},
    {"do", TOKTYPE_DO},
    {"break", TOKTYPE_BREAK},
    {"continue", TOKTYPE_CONTINUE}
};

struct node_s
{
    uint8_t type;
    
    tok_s *tok;
    scope_s *scope;
    
    unsigned index;
    unsigned nchildren;
    node_s **children;
    node_s *parent;
    
    node_s *currtype;
    node_s *statement;
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

struct flow_s
{
    flnode_s *start;
    flnode_s *curr;
    flnode_s *final;
};

struct flnode_s
{
    unsigned nin;
    unsigned nout;
    flnode_s **in;
    flnode_s **out;
};

static tokiter_s *lex(char *src);
static tok_s *mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att);
static bool trykeyword(tokchunk_s **list, tok_s **prev, uint16_t line, char *str);
static void printtoks(tokchunk_s *list);

static tok_s *tok(tokiter_s *ti);
static tok_s *nexttok(tokiter_s *ti);

static node_s *start(tokiter_s *ti);
static void p_statementlist(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_statement(tokiter_s *ti, flow_s *flow);
static node_s *p_expression(tokiter_s *ti, flow_s *flow);
static void p_expression_(tokiter_s *ti, node_s *exp, flow_s *flow);
static node_s *p_simple_expression(tokiter_s *ti, flow_s *flow);
static void p_simple_expression_(tokiter_s *ti, node_s *sexp, flow_s *flow);
static node_s *p_term(tokiter_s *ti, flow_s *flow);
static void p_term_(tokiter_s *ti, node_s *term, flow_s *flow);
static node_s *p_factor(tokiter_s *ti, flow_s *flow);
static void p_optexp(tokiter_s *ti, node_s *factor, flow_s *flow);
static node_s *p_factor_(tokiter_s *ti, flow_s *flow);
static void p_factor__(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_assign(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_lambda(tokiter_s *ti);
static int p_sign(tokiter_s *ti);
static node_s *p_optarglist(tokiter_s *ti, flow_s *flow);
static node_s *p_arglist(tokiter_s *ti, flow_s *flow);
static void p_arglist_(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_control(tokiter_s *ti, flow_s *flow);
static node_s *p_if(tokiter_s *ti, flow_s *flow);
static node_s *p_switch(tokiter_s *ti, flow_s *flow);
static void p_caselist(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_else(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_controlsuffix(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_dec(tokiter_s *ti, flow_s *flow);
static node_s *p_opttype(tokiter_s *ti, flow_s *flow);
static node_s *p_type(tokiter_s *ti,flow_s *flow);
static void p_inh(tokiter_s *ti, node_s *root);
static void p_declist(tokiter_s *ti, node_s *root);
static void p_array(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_optparamlist(tokiter_s *ti, flow_s *flow);
static node_s *p_paramlist(tokiter_s *ti, flow_s *flow);
static void p_paramlist_(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_set(tokiter_s *ti, flow_s *flow);
static void p_optnext(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_set_(tokiter_s *ti, node_s *set, flow_s *flow);
static void synerr_rec(tokiter_s *ti);

static node_s *node_s_(scope_s *scope);
static void addchild(node_s *root, node_s *c);
static node_s *right(node_s *node);
static node_s *left(node_s *node);

static unsigned pjwhash(char *key);
static bool addident(scope_s *root, node_s *ident);
static scope_s *idtinit(scope_s *parent);
static void pushscope(tokiter_s *ti);
static inline void popscope(tokiter_s *ti);
static flow_s *flow_s_(void);
static void addflow(flnode_s *out, flnode_s *in);
static void fflow_stmt(flow_s *flow);
static node_s *getparentfunc(node_s *start);
static node_s *getparentloop(node_s *start);
static void walk_tree(node_s *root);
static bool typeeq(node_s *typeexp, node_s *literal);

static void freetree(node_s *root);


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
                    
                    if(!strcmp(bptr, "true")) {
                        prev = mtok(&curr, line, "1", 1, TOKTYPE_NUM, TOKATT_NUMINT);
                    }
                    else if(!strcmp(bptr, "false")) {
                        prev = mtok(&curr, line, "0", 1, TOKTYPE_NUM, TOKATT_NUMINT);
                    }
                    else if(!trykeyword(&curr, &prev, line, bptr))
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

node_s *start(tokiter_s *ti)
{
    tok_s *t;
    flow_s *flow = flow_s_();
    node_s *root = MAKENODE();
    
    root->type = TYPE_NODE;
    
    ti->scope = idtinit(NULL);
    ti->graph = MAKENODE();
    ti->graph->type = TYPE_NODE;

    p_statementlist(ti, root, flow);
    t = tok(ti);
    
    if(t->type != TOKTYPE_EOF) {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "EOF", NULL);
    }
    
    walk_tree(root);
    
    return root;
}

void p_statementlist(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *statement, *last;
    tok_s *t = tok(ti), *tcmp;
    
    switch(t->type) {
        case TOKTYPE_CLASS:
        case TOKTYPE_VAR:
        case TOKTYPE_LISENER:
        case TOKTYPE_SWITCH:
        case TOKTYPE_FOR:
        case TOKTYPE_WHILE:
        case TOKTYPE_DO:
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
        case TOKTYPE_BREAK:
        case TOKTYPE_CONTINUE:
        case TOKTYPE_SEMICOLON:
            statement = p_statement(ti, flow);
            addchild(root, statement);
            if(statement) {
                if(statement->nchildren) {
                    tcmp = statement->children[0]->tok;
                    if(tcmp) {
                        if(tcmp->type == TOKTYPE_RETURN) {
                            if(!getparentfunc(root)) {
                                adderr(ti, "Return Error", "return from invalid scope", tcmp->line, "return from lambda expression", NULL);
                            }
                        }
                        else if(tcmp->type == TOKTYPE_BREAK) {
                            if(!getparentloop(root)) {
                                adderr(ti, "Break Error", "break from invalid scope", tcmp->line, "break from loop", NULL);

                            }
                        }
                        else if(tcmp->type == TOKTYPE_CONTINUE) {
                            if(!getparentloop(root)) {
                                adderr(ti, "Continue Error", "continue from invalid scope", tcmp->line, "continue from loop", NULL);
                            }
                        }
                    }
                }
            }
            last = statement;
            p_statementlist(ti, root, flow);
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

node_s *p_statement(tokiter_s *ti, flow_s *flow)
{
    node_s *statement, *exp, *jmp;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
        case TOKTYPE_NUM:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NOT:
        case TOKTYPE_IF:
        case TOKTYPE_SWITCH:
        case TOKTYPE_STRING:
        case TOKTYPE_REGEX:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_OPENBRACE:
        case TOKTYPE_ADDOP:
            fflow_stmt(flow);
            return p_expression(ti, flow);
        case TOKTYPE_WHILE:
        case TOKTYPE_DO:
        case TOKTYPE_FOR:
        case TOKTYPE_LISENER:
            return p_control(ti, flow);
        case TOKTYPE_VAR:
        case TOKTYPE_CLASS:
            return p_dec(ti, flow);
        case TOKTYPE_RETURN:
            nexttok(ti);
            statement = MAKENODE();
            statement->type = TYPE_NODE;
            jmp = MAKENODE();
            jmp->type = TYPE_OP;
            jmp->tok = t;
            exp = p_expression(ti, flow);
            addchild(statement, jmp);
            addchild(statement, exp);
            fflow_stmt(flow);
            return statement;
        case TOKTYPE_SEMICOLON:
            nexttok(ti);
            return NULL;
        case TOKTYPE_BREAK:
            nexttok(ti);
            statement = MAKENODE();
            statement->type = TYPE_NODE;
            jmp = MAKENODE();
            jmp->type = TYPE_OP;
            jmp->tok = t;
            addchild(statement, jmp);
            fflow_stmt(flow);
            return statement;
        case TOKTYPE_CONTINUE:
            nexttok(ti);
            statement = MAKENODE();
            statement->type = TYPE_NODE;
            jmp = MAKENODE();
            jmp->type = TYPE_OP;
            jmp->tok = t;
            addchild(statement, jmp);
            fflow_stmt(flow);
            return statement;
        default:
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", "number", "(", "not", "string",
                   "regex", "@", "{", "+", "-", "|", "if", "while", "for", "switch", "var", "return", NULL);
            synerr_rec(ti);
            return NULL;
    }
}

node_s *p_expression(tokiter_s *ti, flow_s *flow)
{
    node_s *node, *exp;
    
    exp = MAKENODE();
    exp->type = TYPE_NODE;

    node = p_simple_expression(ti, flow);
    addchild(exp, node);
    p_expression_(ti, exp, flow);
    
    return exp;
}

void p_expression_(tokiter_s *ti, node_s *exp, flow_s *flow)
{
    node_s *s = NULL, *op = NULL;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_RELOP) {
        nexttok(ti);
        s = p_simple_expression(ti, flow);
        
        op = MAKENODE();
        op->type = TYPE_OP;
        
        addchild(exp, op);
        addchild(exp, s);
    }
}

node_s *p_simple_expression(tokiter_s *ti, flow_s *flow)
{
    int sign = 0;
    node_s *term, *un = NULL, *sexp;
    tok_s *t = tok(ti);
    
    sexp = MAKENODE();
    sexp->type = TYPE_OP;
    
    if(t->type == TOKTYPE_ADDOP)
        sign = p_sign(ti);
    term = p_term(ti, flow);
    
    if(sign == -1) {
        un = MAKENODE();
        un->type = TYPE_OP;
        un->tok = t;
        t->type = TOKTYPE_UNNEG;
        addchild(sexp, un);
    }
    addchild(sexp, term);
    p_simple_expression_(ti, sexp, flow);
    return sexp;
}

void p_simple_expression_(tokiter_s *ti, node_s *sexp, flow_s *flow)
{
    node_s *term = NULL, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ADDOP) {
        nexttok(ti);
        term = p_term(ti, flow);
        
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(sexp, op);
        addchild(sexp, term);
        
        p_simple_expression_(ti, sexp, flow);
    }
}

node_s *p_term(tokiter_s *ti, flow_s *flow)
{
    node_s *f, *term;
    
    term = MAKENODE();
    term->type = TYPE_NODE;
    
    f = p_factor(ti, flow);
    addchild(term, f);
    
    p_term_(ti, term, flow);
    
    return term;
}

void p_term_(tokiter_s *ti, node_s *term, flow_s *flow)
{
    node_s *f = NULL, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_MULOP) {
        nexttok(ti);
        f = p_factor(ti, flow);
        
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(term, op);
        addchild(term, f);
        p_term_(ti, term, flow);
    }
    
}

node_s *p_factor(tokiter_s *ti, flow_s *flow)
{
    node_s *f, *fact;
    
    fact = MAKENODE();
    fact->type = TYPE_NODE;
    
    f = p_factor_(ti, flow);
    addchild(fact, f);
    p_optexp(ti, fact, flow);
    
    return fact;
}

void p_optexp(tokiter_s *ti, node_s *factor, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *f = NULL, *op;
    
    if(t->type == TOKTYPE_EXPOP) {
        nexttok(ti);
        f = p_factor(ti, flow);
        
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(factor, op);
        addchild(factor, f);
    }
}

node_s *p_factor_(tokiter_s *ti, flow_s *flow)
{
    node_s *n, *ident, *f, *op;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
            nexttok(ti);
            
            n = MAKENODE();
            n->type = TYPE_NODE;
            
            ident = MAKENODE();
            ident->type = TYPE_IDENT;
            ident->tok = t;
            
            addchild(n, ident);

            p_factor__(ti, n, flow);
            p_assign(ti, n, flow);
            break;
        case TOKTYPE_NUM:
            nexttok(ti);
            n = MAKENODE();
            n->type = TYPE_NUM;
            n->tok = t;
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            n = p_expression(ti, flow);
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
        case TOKTYPE_NOT:
            nexttok(ti);
            n = MAKENODE();
            n->type = TYPE_NODE;
            
            op = MAKENODE();
            op->type = TYPE_OP;
            op->tok = t;
            
            f = p_factor(ti, flow);
            addchild(n, op);
            addchild(n, f);
            break;
        case TOKTYPE_STRING:
            nexttok(ti);
            n = MAKENODE();
            n->type = TYPE_STRING;
            n->tok = t;
            break;
        case TOKTYPE_REGEX:
            nexttok(ti);
            n = MAKENODE();
            n->type = TYPE_REGEX;
            n->tok = t;
            break;
        case TOKTYPE_OPENBRACE:
            n = p_set(ti, flow);
            break;
        case TOKTYPE_LAMBDA:
            n = p_lambda(ti);
            break;
        case TOKTYPE_IF:
            n = p_if(ti, flow);
            break;
        case TOKTYPE_SWITCH:
            n = p_switch(ti, flow);
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

void p_factor__(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *op, *exp, *ident, *opt;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_OPENBRACKET:
            nexttok(ti);
            exp = p_expression(ti, flow);
            
            op = MAKENODE();
            op->type = TYPE_OP;
            op->tok = t;
        
            addchild(root, op);
            addchild(root, exp);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACKET) {
                nexttok(ti);
                p_factor__(ti, root, flow);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "]", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_DOT:
            op = MAKENODE();
            op->type = TYPE_OP;
            op->tok = t;

            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                nexttok(ti);
                ident = MAKENODE();
                ident->type = TYPE_IDENT;
                ident->tok = t;
                addchild(root, op);
                addchild(root, ident);
                p_factor__(ti, root, flow);
            }
            else {
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            
            op = MAKENODE();
            op->type = TYPE_OP;
            op->tok = t;
            
            opt = p_optarglist(ti, flow);
            addchild(root, op);
            addchild(root, opt);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
                p_factor__(ti, root, flow);
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

void p_assign(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *exp, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ASSIGN) {
        nexttok(ti);
        exp = p_expression(ti, flow);
        
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        
        addchild(root, op);
        addchild(root, exp);
    }
}

node_s *p_lambda(tokiter_s *ti)
{
    flow_s *flow = flow_s_();
    tok_s *t = tok(ti);
    node_s *lambda = MAKENODE(), *op = MAKENODE(), *param, *body = MAKENODE();
    
    lambda->type = TYPE_NODE;
    body->type = TYPE_NODE;
    op->type = TYPE_OP;
    op->tok = t;
    
    t = nexttok(ti);
    
    if(t->type == TOKTYPE_OPENPAREN) {
        nexttok(ti);
        PUSHSCOPE();
        param = p_optparamlist(ti, flow);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEPAREN) {
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                addchild(lambda, op);
                addchild(lambda, param);
                addchild(lambda, body);
                p_statementlist(ti, body, flow);
                flow = flow_s_();
                
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
                lambda = NULL;
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "{", NULL);
                synerr_rec(ti);
            }
        }
        else {
            lambda = NULL;
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
            synerr_rec(ti);
        }
        POPSCOPE();
    }
    else {
        lambda = NULL;
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

node_s *p_optarglist(tokiter_s *ti, flow_s *flow)
{
    node_s *empty;
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
        case TOKTYPE_IF:
        case TOKTYPE_SWITCH:
            return p_arglist(ti, flow);
            break;
        default:
            empty = MAKENODE();
            empty->type = TYPE_NODE;
            //epsilon production
            return empty;
    }
}

node_s *p_arglist(tokiter_s *ti, flow_s *flow)
{
    node_s *root = MAKENODE(), *n;
    
    n = p_expression(ti, flow);
    addchild(root, n);
    p_arglist_(ti, root, flow);

    return root;
}

void p_arglist_(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *n;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        n = p_expression(ti, flow);
        addchild(root, n);
        
        p_arglist_(ti, root, flow);
    }
    else {
        //epsilon production
    }
}

node_s *p_control(tokiter_s *ti, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *control = MAKENODE(), *op = MAKENODE(),
                    *exp, *arg, *ident, *suffix = MAKENODE();
    
    suffix->type = TYPE_NODE;
    
    op->type = TYPE_OP;
    op->tok = t;

    switch(t->type) {
        case TOKTYPE_WHILE:
            nexttok(ti);
            exp = p_expression(ti, flow);
            addchild(control, op);
            addchild(control, exp);
            addchild(control, suffix);
            p_controlsuffix(ti, suffix, flow);
            break;
        case TOKTYPE_FOR:
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                ident = MAKENODE();
                ident->type = TYPE_IDENT;
                ident->tok = t;
                t = nexttok(ti);
                if(t->type == TOKTYPE_FORVAR) {
                    nexttok(ti);
                    exp = p_expression(ti, flow);
                    addchild(control, op);
                    addchild(control, ident);
                    addchild(control, exp);
                    addchild(control, suffix);
                    p_controlsuffix(ti, suffix, flow);
                }
                else {
                    free(control);
                    free(op);
                    free(ident);
                    control = NULL;
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, "<-", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                free(control);
                free(op);
                control = NULL;
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_LISENER:
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENPAREN) {
                nexttok(ti);
                arg = p_arglist(ti, flow);
                t = tok(ti);
                if(t->type == TOKTYPE_CLOSEPAREN) {
                    t = nexttok(ti);
                    if(t->type == TOKTYPE_OPENBRACE) {
                        nexttok(ti);
                        addchild(control, op);
                        addchild(control, arg);
                        addchild(control, suffix);
                        PUSHSCOPE();
                        p_statementlist(ti, suffix, flow);
                        POPSCOPE();
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
                        control = NULL;
                        //syntax error
                        adderr(ti, "Syntax Error", t->lex, t->line, "{", NULL);
                        synerr_rec(ti);
                    }
                }
                else {
                    control = NULL;
                    //syntax error
                    adderr(ti, "Syntax Error", t->lex, t->line, ")", NULL);
                    synerr_rec(ti);
                }
            }
            else {
                control = NULL;
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "(", NULL);
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_DO:
            nexttok(ti);
            addchild(control, op);
            addchild(control, suffix);
            p_controlsuffix(ti, suffix, flow);
            t = tok(ti);
            if(t->type == TOKTYPE_WHILE) {
                nexttok(ti);
                exp = p_expression(ti, flow);
                addchild(control, exp);
            }
            else {
                control = NULL;
                //syntax error
                adderr(ti, "Syntax Error", t->lex, t->line, "while", NULL);
                synerr_rec(ti);
            }
            break;
        default:
            control = NULL;
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "if", "while", "for", "switch", NULL);
            synerr_rec(ti);
            break;
    }
    return control;
}

node_s *p_if(tokiter_s *ti, flow_s *flow)
{
    node_s *root = MAKENODE(), *op = MAKENODE(), *exp, *suffix = MAKENODE();
    flow_s  *ftrue = flow_s_(),
            *ffalse = flow_s_();
    
    root->type = TYPE_NODE;
    suffix->type = TYPE_NODE;
    
    op->type = TYPE_OP;
    op->tok = tok(ti);
    
    nexttok(ti);
    exp = p_expression(ti, flow);
    addchild(root, op);
    addchild(root, exp);
    addchild(root, suffix);
    p_controlsuffix(ti, suffix, flow);
    p_else(ti, root, flow);
    return root;
}

node_s *p_switch(tokiter_s *ti, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *root = MAKENODE(), *op = MAKENODE(), *exp;
    
    root->type = TYPE_NODE;
    op->type = TYPE_OP;
    op->tok = t;
    
    t = nexttok(ti);
    if(t->type == TOKTYPE_OPENPAREN) {
        nexttok(ti);
        exp = p_expression(ti, flow);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEPAREN) {
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                
                addchild(root, op);
                addchild(root, exp);
                
                p_caselist(ti, root, flow);
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
    return root;
}


void p_caselist(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *exp, *arg, *op, *cadef;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_CASE) {
        nexttok(ti);
        
        cadef = MAKENODE();
        cadef->type = TYPE_OP;
        cadef->tok = t;
        
        arg = p_arglist(ti, flow);
        t = tok(ti);
        if(t->type == TOKTYPE_MAP) {
            nexttok(ti);
            
            op = MAKENODE();
            op->type = TYPE_OP;
            op->tok = t;
            
            exp = p_expression(ti, flow);
            addchild(root, cadef);
            addchild(root, arg);
            addchild(root, op);
            addchild(root, exp);
            p_caselist(ti, root, flow);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "->", NULL);
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_DEFAULT) {
        cadef = MAKENODE();
        cadef->type = TYPE_OP;
        cadef->tok = t;
        
        t = nexttok(ti);
        if(t->type == TOKTYPE_MAP) {
            nexttok(ti);
            
            exp = p_expression(ti, flow);
            addchild(root, cadef);
            addchild(root, exp);
            p_caselist(ti, root, flow);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "->", NULL);
            synerr_rec(ti);
        }
    }
}

void p_else(tokiter_s *ti, node_s *root, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *op, *suffix = MAKENODE();

    suffix->type = TYPE_NODE;
    
    if(t->type == TOKTYPE_ELSE) {
        nexttok(ti);
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        addchild(root, op);
        addchild(root, suffix);
        p_controlsuffix(ti, suffix, flow);
    }
}

void p_controlsuffix(tokiter_s *ti, node_s *root, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *statement;
    
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        PUSHSCOPE();
        p_statementlist(ti, root, flow);
        POPSCOPE();
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
        statement = p_statement(ti, flow);
        addchild(root, statement);
    }
}

node_s *p_dec(tokiter_s *ti, flow_s *flow)
{
    node_s *ident, *dectype, *dec, *op;
    tok_s *t = tok(ti);
    
    dec = MAKENODE();
    dec->type = TYPE_NODE;
    
    op = MAKENODE();
    op->type = TYPE_OP;
    op->tok = t;
    
    if(t->type == TOKTYPE_VAR) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            
            ident = MAKENODE();
            ident->type = TYPE_IDENT;
            ident->tok = t;
            
            addchild(dec, op);
            addchild(dec, ident);
            
            dectype = p_opttype(ti, flow);
            addchild(dec, dectype);
            p_assign(ti, dec, flow);
            
            if(addident(ti->scope, ident)) {
               // adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
            }
        }
        else {
            dec = NULL;
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_CLASS) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            
            ident = MAKENODE();
            ident->type = TYPE_IDENT;
            ident->tok = t;
            
            addchild(dec, op);
            addchild(dec, ident);

            if(addident(ti->scope, ident)) {
               // adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
            }
            t = nexttok(ti);
            p_inh(ti, dec);
            t = tok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                PUSHSCOPE();
                p_declist(ti, dec);
                POPSCOPE();
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

node_s *p_opttype(tokiter_s *ti, flow_s *flow)
{
    node_s *type = NULL;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COLON) {
        nexttok(ti);
        type = p_type(ti, flow);
    }
    else {
        //epsilon production
        type = NULL;
    }
    return type;
}

node_s *p_type(tokiter_s *ti, flow_s *flow)
{
    node_s *type, *opt, *ret;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_VOID:
            nexttok(ti);
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok= t;
            break;
        case TOKTYPE_INTEGER:
            nexttok(ti);
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_REAL:
            nexttok(ti);
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_STRINGTYPE:
            nexttok(ti);
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_REGEXTYPE:
            nexttok(ti);
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_LIST:
            nexttok(ti);
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_IDENT:
            nexttok(ti);
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            
            type = MAKENODE();
            type->type = TYPE_TYPEEXP;
            type->tok = t;
            opt = p_optparamlist(ti, flow);
            addchild(type, opt);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
                p_array(ti, type, flow);
                t = tok(ti);
                if(t->type == TOKTYPE_MAP) {
                    nexttok(ti);
                    ret = p_type(ti, flow);
                    addchild(type, ret);
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

void p_inh(tokiter_s *ti, node_s *root)
{
    node_s *op, *ident;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COLON) {
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            ident = MAKENODE();
            ident->type = TYPE_IDENT;
            ident->tok = t;
            addchild(root, op);
            addchild(root, ident);
        }
        else {
            adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
            synerr_rec(ti);
        }
    }
}

void p_declist(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *dec;
    
    while(t->type == TOKTYPE_VAR || t->type == TOKTYPE_CLASS) {
        dec = p_dec(ti, NULL);
        addchild(root, dec);
        t = tok(ti);
        if(t->type == TOKTYPE_SEMICOLON)
            t = nexttok(ti);
    }
}

void p_array(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *exp;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_OPENBRACKET) {
        nexttok(ti);
        
        exp = p_expression(ti, flow);
        addchild(root, exp);
        
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACKET) {
            nexttok(ti);
            p_array(ti, root, flow);
        }
        else {
            //syntax error
            adderr(ti, "Syntax Error", t->lex, t->line, "]", NULL);
            synerr_rec(ti);
        }
    }
}

node_s *p_param(tokiter_s *ti, flow_s *flow)
{
    node_s *n, *ident, *opt;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        nexttok(ti);
        n = MAKENODE();
        n->type = TYPE_NODE;

        ident = MAKENODE();
        ident->type = TYPE_IDENT;
        ident->tok = t;
        
        if(addident(ti->scope, ident)) {
           // adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
        }
        opt = p_opttype(ti, flow);
        addchild(n, ident);
        addchild(n, opt);
        p_assign(ti, n, flow);
        return n;
    }
    else {
        //syntax error
        adderr(ti, "Syntax Error", t->lex, t->line, "identifier", NULL);
        synerr_rec(ti);
        return NULL;
    }
}

node_s *p_optparamlist(tokiter_s *ti, flow_s *flow)
{
    node_s *empty;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        return p_paramlist(ti, flow);
    }
    else {
        //epsilon production
        empty = MAKENODE();
        empty->type = TYPE_NODE;
        return empty;
    }
}

node_s *p_paramlist(tokiter_s *ti, flow_s *flow)
{
    node_s *root = MAKENODE(), *dec;

    root->type = TYPE_PARAMLIST;
    dec = p_param(ti, flow);
    
    addchild(root, dec);
    p_paramlist_(ti, root, flow);
    
    return root;
}

void p_paramlist_(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *dec;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        
        dec = p_param(ti, flow);
        
        addchild(root, dec);
        p_paramlist_(ti, root, flow);
    }
    else {
        //epsilon production
    }
}

node_s *p_set(tokiter_s *ti, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *set = MAKENODE(), *exp;
    
    set->type = TYPE_NODE;
    
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        exp = p_expression(ti, flow);
        addchild(set, exp);
        p_optnext(ti, exp, flow);
        p_set_(ti, set, flow);
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

void p_optnext(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *exp;
    tok_s *t = tok(ti);
    node_s *op;
    
    if(t->type == TOKTYPE_MAP) {
        nexttok(ti);
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        exp = p_expression(ti, flow);
        addchild(root, op);
        addchild(root, exp);
    }
    else if(t->type == TOKTYPE_RANGE) {
        nexttok(ti);
        op = MAKENODE();
        op->type = TYPE_OP;
        op->tok = t;
        exp = p_expression(ti, flow);
        addchild(root, op);
        addchild(root, exp);
    }
}


void p_set_(tokiter_s *ti, node_s *set, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *exp;
    
    if(t->type == TOKTYPE_COMMA) {
        nexttok(ti);
        exp = p_expression(ti, flow);
        p_optnext(ti, exp, flow);
        addchild(set, exp);
        p_set_(ti, set, flow);
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

node_s *node_s_(scope_s *scope)
{
    node_s *e = alloc(sizeof *e);
    
    e->tok = NULL;
    e->index = 0;
    e->scope = scope;
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
        c->parent = root;
        root->children[root->nchildren] = c;
        c->index = root->nchildren;
        root->nchildren++;
    }
}

node_s *right(node_s *node)
{
    unsigned i;
    node_s *p = node->parent;
    
    if(p) {
        i = node->index;
        if(i < node->nchildren - 1)
            return p->children[i + 1];
    }
    return NULL;
}

node_s *left(node_s *node)
{
    unsigned i;
    node_s *p = node->parent;

    if(p) {
        i = node->index;
        if(i)
            return p->children[i - 1];
    }
    return NULL;
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

void pushscope(tokiter_s *ti)
{
    scope_s *o = ti->scope, *n = idtinit(o);
    
    if(o->children)
        o->children = ralloc(o->children, (o->nchildren + 1) * sizeof(*o->children));
    else
        o->children = alloc(sizeof *o->children);
    o->children[o->nchildren] = n;
    o->nchildren++;
    ti->scope = n;
}

inline void popscope(tokiter_s *ti)
{
    ti->scope = ti->scope->parent;
}

void checkvar(node_s *var)
{
    node_s  *id = right(var),
            *opttype,
            *assign = right(id);
    
    if(assign) {
        if(assign->type == TYPE_OP) {
            
        }
        else {
            opttype = assign;
            assign = right(opttype);
            if(assign) {
                
            }
        }
    }
}

flow_s *flow_s_(void)
{
    flow_s *f = alloc(sizeof *f);
    
    f->start = allocz(sizeof *f);
    f->curr = f->start;
    f->final = NULL;
    return f;
}

void addflow(flnode_s *out, flnode_s *in)
{
    if(out->nout)
        out->out = ralloc(out->out, (out->nout + 1) * sizeof(*out->out));
    else
        out->out = alloc(sizeof *out->out);
    out->out[out->nout] = in;
    out->nout++;
    
    if(in->nin)
        in->in = ralloc(in->in, (in->nin + 1) * sizeof(*in->in));
    else
        in->in = alloc(sizeof *in->in);
    in->in[in->nin] = out;
    in->nin++;
}

void fflow_stmt(flow_s *flow)
{
    flnode_s *old = flow->curr;
    flnode_s *nn = allocz(sizeof *nn);
    
    addflow(old, nn);
    flow->curr = nn;
}

node_s *getparentfunc(node_s *start)
{
    tok_s *t;
    
    while(start) {
        if(start->nchildren) {
            t = start->children[0]->tok;
            if(t && t->type == TOKTYPE_LAMBDA)
                return start;
        }
        start = start->parent;
    }
    return NULL;
}

node_s *getparentloop(node_s *start)
{
    tok_s *t;
    
    while(start) {
        if(start->nchildren) {
            t = start->children[0]->tok;
            if(t) {
                switch(t->type) {
                    case TOKTYPE_FOR:
                    case TOKTYPE_WHILE:
                    case TOKTYPE_DO:
                        return start;
                    default:
                        break;
                }
            }
        }
        start = start->parent;
    }
    return NULL;
}

void walk_tree(node_s *root)
{
    unsigned i;
    
    //printf("%u\n", root->nchildren);
    for(i = 0; i < root->nchildren; i++)
        walk_tree(root->children[i]);
    
    if(root->type == TYPE_OP) {
       /* switch(root->tok->type) {
            case TOKTYPE_VAR:
                break;
        }*/
    }
    
}

bool typeeq(node_s *typeexp, node_s *literal)
{
    return false;
}


void freetree(node_s *root)
{
    unsigned i;
    
    for(i = 0; i < root->nchildren; i++)
        freetree(root->children[i]);
    free(root);
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
