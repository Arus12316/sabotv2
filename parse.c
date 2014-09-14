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
                . id <factor''>
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
                <structliteral>
                |
                <mapliteral>
                |
                <lambda> <factor''> { <factor'>.type := closure; <factor'>.val := <lambda>.val }
                |
                <if>
                |
                <switch>

 <factor''> -> [ <expression> ] <factor''> | . id <factor''> | ( <optarglist> ) <factor''> | ε
 
 <assign> -> assignop <expression> | ε
 
 <structliteral> -> ${ <initializerlist> }
 
 <initializerlist> -> <expression> , <initializerlist> | dot id assignop <expression> , <initializerlist> | ε
 
 <mapliteral> -> #{ <maplist> }
 
 <maplist> -> <expression> => <expression> <maplist> | ε
 
 
 <lambda> -> ^ (<optparamlist>) openbrace <statementlist> closebrace
 
 <sign> -> + | -
 
 <optarglist> -> <arglist> | ε
 <arglist> -> <expression> <arglist'>
 <arglist'> -> , <expression> <arglist'> | ε
 
 <control> ->	while <expression> <controlsuffix>
                |
                for id <- <expression> <controlsuffix>
                |
                listener(<optarglist>) <controlsuffix>
                |
                do <controlsuffix> while <expression>
 
 
 <if> -> if <expression> <controlsuffix> <else>
 
 
 <switch> -> switch(<expression>) openbrace <caselist> closebrace
 
 <caselist> ->  case <arglist> map <expression> <caselist>
                |
                default map <expression> <caselist>
                |
                ε
 
 <else> -> else openbrace <statementlist> closebrace | ε
 
 <controlsuffix> -> openbrace <statementlist> closebrace | <statement>
 
 <dec> -> <varlet> id <opttype> <assign> | class id <inh> { <declist> } | enum <optid> { <enumlist> } | struct <optid> { <structdeclarator> }
 
 <varlet> -> var | let
 
 <enumlist> -> id <assign> <enumlist'> | ε
 <enumlist'> -> , id <assign> <enumlist'> | ε
 
 <optid> -> id | ε
 
 <structdeclarator> -> id : type , <structdeclarator> | ε
 
 <opttype> -> : <type> | ε
 
 <type> -> void | int <array> | char <array> | real <array> | string <array> | 
            regex <array> | Map <array> <optmaptype> |
            id <array> | ( <typelist> ) <array> map <type>
 
 <optmaptype> -> openbrace <type> => <type> closebrace
 
 <inh> -> : id | ε
 <declist> -> <dec> <optsemicolon> <declist> | ε

 <optsemicolon> -> ; | ε
 
 <array> -> [ <expression> ] <array> | ε
 
 <typelist> -> <optname> <type> <typelist'> | vararg | ε
 <typelist'> -> , <optname> <type> <typelist'> | vararg | ε
 <optname> -> id : | ε
 
 <optparamlist> -> <paramlist> | ε
 <param> -> id <opttype> <assign>
 <paramlist> -> <param> <paramlist'> | vararg
 <paramlist'> -> , <param> <paramlist'> | vararg | ε
 
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
#define ERR(str) adderr(ti,str,-1)

#define SYM_TABLE_SIZE 19

enum {
    TOKTYPE_EOF,
    TOKTYPE_IDENT,
    TOKTYPE_NUM,
    TOKTYPE_STRING,
    TOKTYPE_COMMA,
    TOKTYPE_DOT,
    TOKTYPE_DMAP,
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
    TOKTYPE_ENUM,
    TOKTYPE_NOT,
    TOKTYPE_IF,
    TOKTYPE_ELSE,
    TOKTYPE_SWITCH,
    TOKTYPE_CASE,
    TOKTYPE_DEFAULT,
    TOKTYPE_DO,
    TOKTYPE_VARARG,
    TOKTYPE_WHILE,
    TOKTYPE_FOR,
    TOKTYPE_LISTENER,
    TOKTYPE_VAR,
    TOKTYPE_LET,
    TOKTYPE_VOID,
    TOKTYPE_INTEGER,
    TOKTYPE_REAL,
    TOKTYPE_STRINGTYPE,
    TOKTYPE_REGEXTYPE,
    TOKTYPE_RETURN,
    TOKTYPE_BREAK,
    TOKTYPE_CONTINUE,
    TOKTYPE_CLASS,
    TOKTYPE_CHAR,
    TOKTYPE_CHARTYPE,
    TOKTYPE_UNNEG,
    TOKTYPE_STRUCTLITERAL,
    TOKTYPE_MAPLITERAL,
    TOKTYPE_STRUCTTYPE,
    TOKTYPE_MAPTYPE
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
    TOKATT_EXP,
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
typedef struct edge_s edge_s;

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
    buf_s *code;
    unsigned labelcount;
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
    {"listener", TOKTYPE_LISTENER},
    {"var", TOKTYPE_VAR},
    {"let", TOKTYPE_LET},
    {"void", TOKTYPE_VOID},
    {"int", TOKTYPE_INTEGER},
    {"char", TOKTYPE_CHARTYPE},
    {"real", TOKTYPE_REAL},
    {"string", TOKTYPE_STRINGTYPE},
    {"regex", TOKTYPE_REGEXTYPE},
    {"return", TOKTYPE_RETURN},
    {"class", TOKTYPE_CLASS},
    {"do", TOKTYPE_DO},
    {"break", TOKTYPE_BREAK},
    {"continue", TOKTYPE_CONTINUE},
    {"enum", TOKTYPE_ENUM},
    {"struct", TOKTYPE_STRUCTTYPE},
    {"map", TOKTYPE_MAPTYPE}
};

struct node_s
{
    uint8_t ntype;
    uint8_t stype;
    node_s *ctype;
    bool branch_complete;
    tok_s *tok;
    scope_s *scope;
    unsigned index;
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
    edge_s **in;
    edge_s **out;
};

struct edge_s
{
    node_s *value;
    flnode_s *out;
    flnode_s *in;
};

static tokiter_s *lex(char *src);
static tok_s *mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att);
static bool trykeyword(tokchunk_s **list, tok_s **prev, uint16_t line, char *str);
static void printtoks(tokchunk_s *list);

static tok_s *tok(tokiter_s *ti);
static tok_s *nexttok(tokiter_s *ti);
static tok_s *peeknexttok(tokiter_s *ti);

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
static node_s *p_optexp(tokiter_s *ti, node_s *fact, flow_s *flow);
static node_s *p_factor_(tokiter_s *ti, flow_s *flow);
static void p_factor__(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_assign(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_structliteral(tokiter_s *ti);
static void p_initializerlist(tokiter_s *ti);
static void p_mapliteral(tokiter_s *ti);
static void p_maplist(tokiter_s *ti);
static node_s *p_lambda(tokiter_s *ti);
static int p_sign(tokiter_s *ti);
static node_s *p_optarglist(tokiter_s *ti, flow_s *flow);
static node_s *p_arglist(tokiter_s *ti, flow_s *flow);
static void p_arglist_(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_control(tokiter_s *ti, flow_s *flow);
static node_s *p_if(tokiter_s *ti, flow_s *flow);
static void p_else(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_switch(tokiter_s *ti, flow_s *flow);
static void p_caselist(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_controlsuffix(tokiter_s *ti, node_s *root, flow_s *flow);
static node_s *p_dec(tokiter_s *ti, flow_s *flow);
static void p_enumlist(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_enumlist_(tokiter_s *ti, node_s *root, flow_s *flow);
static void p_optid(tokiter_s *ti, node_s *root);
static void p_structdeclarator(tokiter_s *ti);
static node_s *p_opttype(tokiter_s *ti, flow_s *flow);
static node_s *p_type(tokiter_s *ti,flow_s *flow);
static void p_inh(tokiter_s *ti, node_s *root);
static void p_declist(tokiter_s *ti, node_s *root);
static void p_array(tokiter_s *ti, node_s *root, flow_s *flow);

static node_s *p_typelist(tokiter_s *ti, flow_s *flow);
static void p_typelist_(tokiter_s *ti, node_s *root, flow_s *flow);

static node_s *p_optparamlist(tokiter_s *ti, flow_s *flow);
static node_s *p_paramlist(tokiter_s *ti, flow_s *flow);
static void p_paramlist_(tokiter_s *ti, node_s *root, flow_s *flow);
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
static edge_s *edge_s_(node_s *stmt);
static flow_s *flow_s_(void);
static flnode_s *flnode_s_(void);
static bool checkflow(node_s *root);
static void addflow(flnode_s *out, node_s *stmt, flnode_s *in);
static void flow_reparent(flnode_s *f1, flnode_s *f2);
static node_s *getparentfunc(node_s *start);
static node_s *getparentloop(node_s *start);
static void walk_tree(node_s *root);
static bool typeeq(node_s *typeexp, node_s *literal);

static void freetree(node_s *root);

static void emit(tokiter_s *ti, char *code, ...);
static void makelabel(tokiter_s *ti, char *buf);

static void adderr(tokiter_s *ti, char *str, int lineno);

errlist_s *parse(char *src)
{
    tokiter_s *ti;
    
    ti = lex(src);
    
    start(ti);
    
    printf("code: %s\n", ti->code->buf);
    
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
                if(*(src + 1) == '.' && *(src + 2) == '.') {
                    prev = mtok(&curr, line, "...", 1, TOKTYPE_VARARG, TOKATT_DEFAULT);
                    src += 3;
                }
                else {
                    prev = mtok(&curr, line, ".", 1, TOKTYPE_DOT, TOKATT_DEFAULT);
                    src++;
                }
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
            case '@':
                prev = mtok(&curr, line, "@", 1, TOKTYPE_LAMBDA, TOKATT_DEFAULT);
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
            case '$':
                if(*(src + 1) == '{') {
                    prev = mtok(&curr, line, "${", 1, TOKTYPE_STRUCTLITERAL, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    adderr(ti, "Lexical Error: Expected '{' for '${", line);
                }
                break;
            case '#':
                if(*(src + 1) == '{') {
                    prev = mtok(&curr, line, "!", 1, TOKTYPE_MAPLITERAL, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    adderr(ti, "Lexical Error: Expected '{' for '${", line);
                }
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
                if(*(src + 1) == '/') {
                    do {
                        src++;
                    }
                    while(*src && *src != '\n');
                }
                else {
                    if(prev) {
                        switch(prev->type) {
                            case TOKTYPE_NUM:
                            case TOKTYPE_CHAR:
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
                                //adderr(ti, "Lexical Error", "EOF", line, "/", NULL);
                                adderr(ti, "Lexical Error: Expected '/' but got EOF", line);
                                break;
                            }
                        }
                        c = *++src;
                        *src = '\0';
                        prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_REGEX, TOKATT_DEFAULT);
                        *src = c;
                    }
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
                    prev = mtok(&curr, line, "=>", 2, TOKTYPE_DMAP, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "=", 1, TOKTYPE_RELOP, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case '\'':
                bptr = src++;
                while(*src != '\'') {
                    if(*src)
                        src++;
                    else {
                        adderr(ti, "Lexical Error: Expected ' but got EOF", line);
                        break;
                    }
                }
                c = *++src;
                *src = '\0';
                prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_CHAR, TOKATT_DEFAULT);
                *src = c;
                break;
            case '"':
                bptr = src++;
                while(*src != '"') {
                    if(*src)
                        src++;
                    else {
                        adderr(ti, "Lexical Error: Expected \" but got EOF", line);
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
                                adderr(ti, "Lexical Error: Invalid number", line);
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
                else if(isalpha(*src) || *src == '_') {
                    bptr = src++;
                    while(isalnum(*src) || *src == '_')
                        src++;
                    c = *src;
                    *src = '\0';
                    
                    if(!strcmp(bptr, "true")) {
                        prev = mtok(&curr, line, "1", 1, TOKTYPE_NUM, TOKATT_NUMINT);
                    }
                    else if(!strcmp(bptr, "false")) {
                        prev = mtok(&curr, line, "0", 1, TOKTYPE_NUM, TOKATT_NUMINT);
                    }
                    else if(!trykeyword(&curr, &prev, line, bptr)) {
                        prev = mtok(&curr, line, bptr, src - bptr, TOKTYPE_IDENT, TOKATT_DEFAULT);
                    }
                    *src = c;
                }
                else {
                    c = *++src;
                    *src = '\0';
                    adderr(ti, "Lexical Error: Unknown Language Character", line);
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
    else if(i == ti->curr->size - 1) {
        ti->curr = ti->curr->next;
        ti->i = 0;
        return &ti->curr->tok[0];
    }
    perror("nexttok error");
    return &eoftok;
}

tok_s *peeknexttok(tokiter_s *ti)
{
    uint8_t i = ti->i;
    
    if(i < ti->curr->size-1) {
        return &ti->curr->tok[i + 1];
    }
    else if(i == ti->curr->size - 1) {
        return &ti->curr->next->tok[0];
    }
    return &eoftok;
}

node_s *start(tokiter_s *ti)
{
    tok_s *t;
    flow_s *flow = flow_s_();
    node_s *root = MAKENODE();
    
    root->ntype = TYPE_NODE;
    
    ti->scope = idtinit(NULL);
    ti->graph = MAKENODE();
    ti->graph->ntype = TYPE_NODE;
    ti->code = bufinit();
    ti->labelcount = 0;

    p_statementlist(ti, root, flow);
    t = tok(ti);
    
    if(t->type != TOKTYPE_EOF) {
        //syntax error
        ERR("Syntax Error: Expected EOF");
    }
    
    return root;
}

void p_statementlist(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *statement, *last;
    tok_s *t = tok(ti), *tcmp;
    
    switch(t->type) {
        case TOKTYPE_CLASS:
        case TOKTYPE_VAR:
        case TOKTYPE_LET:
        case TOKTYPE_ENUM:
        case TOKTYPE_LISTENER:
        case TOKTYPE_SWITCH:
        case TOKTYPE_FOR:
        case TOKTYPE_WHILE:
        case TOKTYPE_DO:
        case TOKTYPE_IF:
        case TOKTYPE_ADDOP:
        case TOKTYPE_OPENBRACE:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_REGEX:
        case TOKTYPE_CHAR:
        case TOKTYPE_STRING:
        case TOKTYPE_NOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NUM:
        case TOKTYPE_STRUCTLITERAL:
        case TOKTYPE_MAPLITERAL:
        case TOKTYPE_DOT:
        case TOKTYPE_IDENT:
        case TOKTYPE_RETURN:
        case TOKTYPE_BREAK:
        case TOKTYPE_CONTINUE:
        case TOKTYPE_STRUCTTYPE:
        case TOKTYPE_SEMICOLON:
            statement = p_statement(ti, flow);
            addchild(root, statement);
            if(statement) {
                if(statement->nchildren) {
                    tcmp = statement->children[0]->tok;
                    if(tcmp) {
                        if(tcmp->type == TOKTYPE_RETURN) {
                            if(!getparentfunc(root)) {
                                ERR("Error: return from non-closure block");
                            }
                        }
                        else if(tcmp->type == TOKTYPE_BREAK) {
                            if(!getparentloop(root)) {
                                ERR("Error: break from non-loop block");
                            }
                        }
                        else if(tcmp->type == TOKTYPE_CONTINUE) {
                            if(!getparentloop(root)) {
                                ERR("Error: continue from non-loop block");
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
            flow->final = flow->curr;
            //epsilon production
            break;
        default:
            ERR("Syntax Error: Expected class, var, listener, switch, for, while, if, +, -, {, }, ^{, \
                regex, string, !, (, number, identifier, or return");
            synerr_rec(ti);
            break;
    }
}

node_s *p_statement(tokiter_s *ti, flow_s *flow)
{
    flnode_s *fn = flnode_s_();
    node_s *statement, *exp, *jmp;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
        case TOKTYPE_NUM:
        case TOKTYPE_DOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NOT:
        case TOKTYPE_IF:
        case TOKTYPE_SWITCH:
        case TOKTYPE_CHAR:
        case TOKTYPE_STRING:
        case TOKTYPE_REGEX:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_OPENBRACE:
        case TOKTYPE_STRUCTLITERAL:
        case TOKTYPE_MAPLITERAL:
        case TOKTYPE_ADDOP:
            statement = p_expression(ti, flow);
            addflow(flow->curr, statement, fn);
            flow->curr = fn;
            return statement;
        case TOKTYPE_WHILE:
        case TOKTYPE_DO:
        case TOKTYPE_FOR:
        case TOKTYPE_LISTENER:
            statement =  p_control(ti, flow);
            addflow(flow->curr, statement, fn);
            flow->curr = fn;
            return statement;
        case TOKTYPE_VAR:
        case TOKTYPE_LET:
        case TOKTYPE_CLASS:
        case TOKTYPE_ENUM:
        case TOKTYPE_STRUCTTYPE:
            addflow(flow->curr, NULL, fn);
            flow->curr = fn;
            return p_dec(ti, flow);
        case TOKTYPE_RETURN:
            nexttok(ti);
            statement = MAKENODE();
            statement->ntype = TYPE_NODE;
            jmp = MAKENODE();
            jmp->ntype = TYPE_OP;
            jmp->tok = t;
            exp = p_expression(ti, flow);
            statement->stype = exp->stype;
            statement->ctype = exp->ctype;
            statement->branch_complete = exp->branch_complete;
            addchild(statement, jmp);
            addchild(statement, exp);
            addflow(flow->curr, exp, fn);
            flow->final = fn;
            return statement;
        case TOKTYPE_SEMICOLON:
            nexttok(ti);
            statement = MAKENODE();
            statement->ntype = TYPE_NODE;
            statement->stype = TYPE_VOID;
            statement->ctype = NULL;
            statement->branch_complete = false;
            return statement;
        case TOKTYPE_BREAK:
            nexttok(ti);
            statement = MAKENODE();
            statement->ntype = TYPE_NODE;
            statement->stype = TYPE_VOID;
            statement->ctype = NULL;
            jmp = MAKENODE();
            jmp->ntype = TYPE_OP;
            jmp->tok = t;
            addchild(statement, jmp);
            addflow(flow->curr, statement, fn);
            flow->curr = fn;
            return statement;
        case TOKTYPE_CONTINUE:
            nexttok(ti);
            statement = MAKENODE();
            statement->ntype = TYPE_NODE;
            statement->stype = TYPE_VOID;
            statement->ctype = NULL;
            jmp = MAKENODE();
            jmp->ntype = TYPE_OP;
            jmp->tok = t;
            addchild(statement, jmp);
            addflow(flow->curr, statement, fn);
            flow->curr = fn;
            return statement;
        default:
            //syntax error
            ERR("Syntax Error: Expected identifier, number, (, not, string, regex, @, {, +, -, !, if, while \
                for, switch, var, or return");
            synerr_rec(ti);
            return NULL;
    }
}

node_s *p_expression(tokiter_s *ti, flow_s *flow)
{
    node_s *node, *exp;
    
    exp = MAKENODE();
    exp->ntype = TYPE_NODE;
    
    node = p_simple_expression(ti, flow);
    addchild(exp, node);
    p_expression_(ti, exp, flow);
    
    exp->branch_complete = node->branch_complete;
    exp->stype = node->stype;
    exp->ctype = node->ctype;
    
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
        op->ntype = TYPE_OP;
        
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
    sexp->ntype = TYPE_OP;
    
    if(t->type == TOKTYPE_ADDOP)
        sign = p_sign(ti);
    term = p_term(ti, flow);
    
    if(sign == -1) {
        un = MAKENODE();
        un->ntype = TYPE_OP;
        un->tok = t;
        t->type = TOKTYPE_UNNEG;
        addchild(sexp, un);
    }
    addchild(sexp, term);
    p_simple_expression_(ti, sexp, flow);
    
    sexp->branch_complete = term->branch_complete;
    sexp->stype = term->stype;
    sexp->ctype = term->ctype;
    
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
        op->ntype = TYPE_OP;
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
    term->ntype = TYPE_NODE;
    
    f = p_factor(ti, flow);
    addchild(term, f);
    
    p_term_(ti, term, flow);
    
    term->branch_complete = f->branch_complete;
    term->stype = f->stype;
    term->ctype = f->ctype;
    
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
        op->ntype = TYPE_OP;
        op->tok = t;
        
        addchild(term, op);
        addchild(term, f);
        p_term_(ti, term, flow);
    }
}

node_s *p_factor(tokiter_s *ti, flow_s *flow)
{
    node_s *f, *fact, *expon;
    
    fact = MAKENODE();
    fact->ntype = TYPE_NODE;
    
    f = p_factor_(ti, flow);
    
    if(f)
        fact->branch_complete = f->branch_complete;
    
    
    addchild(fact, f);
    expon = p_optexp(ti, fact, flow);
    
    fact->stype = f->stype;
    fact->ctype = f->ctype;
    
    return fact;
}

node_s *p_optexp(tokiter_s *ti, node_s *fact, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s  *f, *op;
    
    if(t->type == TOKTYPE_EXPOP) {
        nexttok(ti);
        f = p_factor(ti, flow);
        op = MAKENODE();
        op->ntype = TYPE_OP;
        op->tok = t;
        addchild(fact, op);
        addchild(fact, f);
        
        if(f && f->branch_complete) {
            if(f->stype == TYPE_VOID) {
                adderr(ti, "Error: Right operand of exponent contains branches that evaluate to different incompatible types", 0);
            }
            else if(f->stype == TYPE_LIST) {
                adderr(ti, "Error: Right operand of exponent contains incompatible list type", 0);
            }
        }
        else {
            adderr(ti, "Error: Right operand of exponent contains branch that evaluates to void statement", 0);
        }
    }
    else {
        f = NULL;
    }
    return f;
}

node_s *p_factor_(tokiter_s *ti, flow_s *flow)
{
    flnode_s *fl = flnode_s_();
    node_s *n, *ident, *f, *op;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
            nexttok(ti);
            
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            
            ident = MAKENODE();
            ident->ntype = TYPE_IDENT;
            ident->tok = t;
            
            addchild(n, ident);

            p_factor__(ti, n, flow_s_());
            p_assign(ti, n, flow_s_());
            
            addflow(flow->curr, ident, fl);
            flow->curr = fl;
            break;
        case TOKTYPE_DOT:
            op = MAKENODE();
            op->ntype = TYPE_OP;
            op->tok = t;
            t = nexttok(ti);

            n = MAKENODE();
            n->ntype = TYPE_NODE;

            if(t->type == TOKTYPE_IDENT) {
                nexttok(ti);
                ident = MAKENODE();
                ident->ntype = TYPE_IDENT;
                ident->tok = t;
                addchild(n, op);
                addchild(n, ident);
                p_factor__(ti, n, flow_s_());
                
                addflow(flow->curr, ident, fl);
                flow->curr = fl;
            }
            else {
                n = NULL;
                //syntax error
                ERR("Syntax Error: Expected identifier");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_NUM:
            nexttok(ti);
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            n->tok = t;
            if(t->att == TOKATT_NUMINT)
                n->stype = TYPE_INTEGER;
            else
                n->stype = TYPE_REAL;
            n->ctype = NULL;
            addflow(flow->curr, n, fl);
            flow->curr = fl;
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
                ERR("Syntax Error: Expected )");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_NOT:
            nexttok(ti);
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            n->stype = TYPE_INTEGER;
            n->ctype = NULL;
            
            op = MAKENODE();
            op->ntype = TYPE_OP;
            op->tok = t;
            
            f = p_factor(ti, flow);
            addchild(n, op);
            addchild(n, f);
            break;
        case TOKTYPE_CHAR:
            nexttok(ti);
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            n->stype = TYPE_INTEGER;
            n->ctype = NULL;
            n->tok = t;
            break;
        case TOKTYPE_STRING:
            nexttok(ti);
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            n->stype = TYPE_STRING;
            n->ctype = NULL;
            n->tok = t;
            break;
        case TOKTYPE_REGEX:
            nexttok(ti);
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            n->stype = TYPE_REGEX;
            n->ctype = NULL;
            n->tok = t;
            break;
        case TOKTYPE_STRUCTLITERAL:
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            n->stype = TYPE_NUM;
            n->ctype = NULL;
            n->tok = t;
            p_structliteral(ti);
            break;
        case TOKTYPE_MAPLITERAL:
            n = MAKENODE();
            n->ntype = TYPE_NODE;
            n->stype = TYPE_NUM;
            n->ctype = NULL;
            n->tok = t;
            p_mapliteral(ti);
            break;
        case TOKTYPE_LAMBDA:
            n = p_lambda(ti);
            p_factor__(ti, n, flow_s_());
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
            ERR("Syntax Error: Expected identifier, number, (, string, regex, {, @");
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
            op->ntype = TYPE_OP;
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
                ERR("Syntax Error: Expected ]");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_DOT:
            op = MAKENODE();
            op->ntype = TYPE_OP;
            op->tok = t;
            
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                nexttok(ti);
                ident = MAKENODE();
                ident->ntype = TYPE_IDENT;
                ident->tok = t;
                addchild(root, op);
                addchild(root, ident);
                p_factor__(ti, root, flow);
            }
            else {
                //syntax error
                ERR("Syntax Error: Expected identifier");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            
            op = MAKENODE();
            op->ntype = TYPE_OP;
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
                ERR("Syntax Error: Expected )");
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
        op->ntype = TYPE_OP;
        op->tok = t;
        
        addchild(root, op);
        addchild(root, exp);
    }
}

void p_structliteral(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_STRUCTLITERAL) {
        nexttok(ti);
        p_initializerlist(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACE) {
            nexttok(ti);
        }
        else {
            ERR("Syntax Error: Expected } but got:");
            synerr_rec(ti);
        }
    }
    else {
        ERR("Syntax Error: Expected ${ but got:");
        synerr_rec(ti);
    }
}

void p_initializerlist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_SWITCH:
        case TOKTYPE_IF:
        case TOKTYPE_ADDOP:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_MAPLITERAL:
        case TOKTYPE_STRUCTLITERAL:
        case TOKTYPE_REGEX:
        case TOKTYPE_STRING:
        case TOKTYPE_NOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NUM:
        case TOKTYPE_IDENT:
            p_expression(ti, flow_s_());
            t = tok(ti);
            if(t->type == TOKTYPE_COMMA) {
                nexttok(ti);
                p_initializerlist(ti);
            }
            break;
        case TOKTYPE_DOT:
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                t = nexttok(ti);
                if(t->type == TOKTYPE_ASSIGN) {
                    nexttok(ti);
                    p_expression(ti, flow_s_());
                    t = tok(ti);
                    if(t->type == TOKTYPE_COMMA) {
                        nexttok(ti);
                        p_initializerlist(ti);
                    }
                }
                else {
                    ERR("Syntax Error: Expected :=");
                    synerr_rec(ti);
                }
            }
            else {
                ERR("Syntax Error: Expected identifier");
                synerr_rec(ti);
            }
            break;
        default:
            //epsilon production
            break;
    }
    
}

void p_mapliteral(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_MAPLITERAL) {
        nexttok(ti);
        p_maplist(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACE) {
            nexttok(ti);
        }
        else {
            ERR("Syntax Error: Expected } but got:");
            synerr_rec(ti);
        }
    }
    else {
        ERR("Syntax Error: Expected #{ but got:");
        synerr_rec(ti);
    }
}

void p_maplist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_SWITCH:
        case TOKTYPE_IF:
        case TOKTYPE_ADDOP:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_MAPLITERAL:
        case TOKTYPE_STRUCTLITERAL:
        case TOKTYPE_REGEX:
        case TOKTYPE_STRING:
        case TOKTYPE_NOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NUM:
        case TOKTYPE_DOT:
        case TOKTYPE_IDENT:
            p_expression(ti, flow_s_());
            t = tok(ti);
            if(t->type == TOKTYPE_DMAP) {
                nexttok(ti);
                p_expression(ti, flow_s_());
                t = tok(ti);
                if(t->type == TOKTYPE_COMMA) {
                    nexttok(ti);
                    p_maplist(ti);
                }
            }
            else {
                ERR("Syntax Error: Expected =>");
                synerr_rec(ti);
            }
            break;
        default:
            //epsilon production
            break;
    }
}

node_s *p_lambda(tokiter_s *ti)
{
    flow_s *flow = flow_s_();
    tok_s *t = tok(ti);
    node_s *lambda = MAKENODE(), *op = MAKENODE(), *param, *body = MAKENODE();
    
    lambda->ntype = TYPE_NODE;
    body->ntype = TYPE_NODE;
    op->ntype = TYPE_OP;
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
                    ERR("Syntax Error: Expected }");
                    synerr_rec(ti);
                }
            }
            else {
                lambda = NULL;
                //syntax error
                ERR("Syntax Error: Expected {");
                synerr_rec(ti);
            }
        }
        else {
            lambda = NULL;
            //syntax error
            ERR("Syntax Error: Expected )");
            synerr_rec(ti);
        }
        POPSCOPE();
    }
    else {
        lambda = NULL;
        //syntax error
        ERR("Syntax Error: Expected (");
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
        ERR("Syntax Error: Expected + or -");
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
        case TOKTYPE_CHAR:
        case TOKTYPE_STRING:
        case TOKTYPE_NOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NUM:
        case TOKTYPE_DOT:
        case TOKTYPE_IDENT:
        case TOKTYPE_IF:
        case TOKTYPE_SWITCH:
            return p_arglist(ti, flow);
            break;
        default:
            empty = MAKENODE();
            empty->ntype = TYPE_NODE;
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
    
    control->ntype = TYPE_NODE;
    control->stype = TYPE_VOID;
    control->ctype = NULL;
    suffix->ntype = TYPE_NODE;
    
    op->ntype = TYPE_OP;
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
                ident->ntype = TYPE_IDENT;
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
                    control = NULL;
                    //syntax error
                    ERR("Syntax Error: Expected <-");
                    synerr_rec(ti);
                }
            }
            else {
                control = NULL;
                //syntax error
                ERR("Syntax Error: Expected identifier");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_LISTENER:
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENPAREN) {
                nexttok(ti);
                arg = p_optarglist(ti, flow);
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
                            ERR("Syntax Error: Expected )");
                            synerr_rec(ti);
                        }
                    }
                    else {
                        control = NULL;
                        //syntax error
                        ERR("Syntax Error: Expected {");
                        synerr_rec(ti);
                    }
                }
                else {
                    control = NULL;
                    //syntax error
                    ERR("Syntax Error: Expected )");
                    synerr_rec(ti);
                }
            }
            else {
                control = NULL;
                //syntax error
                ERR("Syntax Error: Expected (");
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
                ERR("Syntax Error: Expected while");
                synerr_rec(ti);
            }
            break;
        default:
            control = NULL;
            //syntax error
            ERR("Syntax Error: Expected if, while, for, or switch");
            synerr_rec(ti);
            break;
    }
    return control;
}

node_s *p_if(tokiter_s *ti, flow_s *flow)
{
    char label1[16], label2[16];
    flnode_s *cond = flnode_s_(), *final = flnode_s_();
    node_s *root = MAKENODE(), *op = MAKENODE(), *exp, *suffix = MAKENODE(), *esuffix;
    
    makelabel(ti, label1);
    makelabel(ti, label2);
    
    root->ntype = TYPE_NODE;
    suffix->ntype = TYPE_NODE;
    
    op->ntype = TYPE_OP;
    op->tok = tok(ti);
    
    nexttok(ti);
    exp = p_expression(ti, flow);
    emit(ti, "\ttest: exp\n", NULL);
    emit(ti, "\tbne ", label1, NULL);
    emit(ti, "\t...\n", NULL);
    emit(ti, "\tjmp ", label2, NULL);
    emit(ti, label1, "\n\t...\n", label2, NULL);
    
    addchild(root, op);
    addchild(root, exp);
    addchild(root, suffix);
    addflow(flow->curr, exp, cond);
    flow->curr = cond;
    p_controlsuffix(ti, suffix, flow);
    
    addflow(flow->curr, NULL, final);
    flow->curr = cond;
    p_else(ti, root, flow);
    addflow(flow->curr, NULL, final);
    flow->curr = final;
    
    if(root->nchildren > 4) {
        esuffix = root->children[4];
        
        root->branch_complete = suffix->branch_complete && esuffix->branch_complete;
        
        if(esuffix->stype == suffix->stype) {
            root->stype = suffix->stype;
        }
        else if((suffix->stype == TYPE_REAL && esuffix->stype == TYPE_INTEGER) || (suffix->stype == TYPE_INTEGER && esuffix->stype == TYPE_REAL)) {
            root->stype = TYPE_REAL;
        }
        else {
            printf("suffix: %d\tesuffix: %d\n", suffix->stype, esuffix->stype);
            root->stype = TYPE_VOID;
        }
    }
    else {
        root->stype = suffix->stype;
        root->ctype = suffix->ctype;
        root->branch_complete = false;
    }
    
    return root;
}

bool checkflow(node_s *root)
{
    if(root->nchildren > 4) {
        return true;
    }
    return false;
}

void p_else(tokiter_s *ti, node_s *root, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *op, *suffix = MAKENODE();
    
    suffix->ntype = TYPE_NODE;
    
    if(t->type == TOKTYPE_ELSE) {
        nexttok(ti);
        op = MAKENODE();
        op->ntype = TYPE_OP;
        op->tok = t;
        addchild(root, op);
        addchild(root, suffix);
        p_controlsuffix(ti, suffix, flow);
        addflow(flow->curr, NULL, flow->final);
    }
}

node_s *p_switch(tokiter_s *ti, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *root = MAKENODE(), *op = MAKENODE(), *exp;
    
    root->ntype = TYPE_NODE;
    op->ntype = TYPE_OP;
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
                    ERR("Syntax Error: Expected }");
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                ERR("Syntax Error: Expected {");
                synerr_rec(ti);
            }
        }
        else {
            //syntax error
            ERR("Syntax Error: Expected )");
            synerr_rec(ti);
        }
    }
    else {
        //syntax error
        ERR("Syntax Error: Expected (");
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
        cadef->ntype = TYPE_OP;
        cadef->tok = t;
        
        arg = p_arglist(ti, flow);
        t = tok(ti);
        if(t->type == TOKTYPE_MAP) {
            nexttok(ti);
            
            op = MAKENODE();
            op->ntype = TYPE_OP;
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
            ERR("Syntax Error: Expected ->");
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_DEFAULT) {
        cadef = MAKENODE();
        cadef->ntype = TYPE_OP;
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
            ERR("Syntax Error: Expected ->");
            synerr_rec(ti);
        }
    }
}

void p_controlsuffix(tokiter_s *ti, node_s *root, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *statement, *last;
    
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        PUSHSCOPE();
        p_statementlist(ti, root, flow);
        POPSCOPE();
        if(root->nchildren) {
            last = root->children[root->nchildren - 1];
            root->stype = last->stype;
            root->ctype = last->ctype;
            root->branch_complete = last->branch_complete;
        }
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACE) {
            nexttok(ti);
        }
        else {
            //syntax error
            ERR("Syntax Error: Expected }");
            synerr_rec(ti);
        }
    }
    else {
        statement = p_statement(ti, flow);
        assert(statement->stype != TYPE_VOID);
        addchild(root, statement);
        root->branch_complete = statement->branch_complete;
        root->stype = statement->stype;
        root->ctype = statement->ctype;
    }
}

node_s *p_dec(tokiter_s *ti, flow_s *flow)
{
    node_s *ident, *dectype, *dec, *op;
    tok_s *t = tok(ti);
    
    dec = MAKENODE();
    dec->ntype = TYPE_NODE;
    dec->stype = TYPE_VOID;
    dec->ctype = NULL;
    
    op = MAKENODE();
    op->ntype = TYPE_OP;
    op->tok = t;
    
    if(t->type == TOKTYPE_VAR || t->type == TOKTYPE_LET) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            
            ident = MAKENODE();
            ident->ntype = TYPE_IDENT;
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
            ERR("Syntax Error: Expected identifier");
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_CLASS) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            
            ident = MAKENODE();
            ident->ntype = TYPE_IDENT;
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
                    ERR("Syntax Error: Expected identifier");
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                ERR("Syntax Error: Expected {");
                synerr_rec(ti);
            }
        }
        else {
            //syntax error
            ERR("Syntax Error: Expected identifier");
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_ENUM) {
        nexttok(ti);
        
        ident = MAKENODE();
        ident->ntype = TYPE_IDENT;
        ident->tok = t;
        
        p_optid(ti, ident);
        t = tok(ti);
        
        if(t->type == TOKTYPE_OPENBRACE) {
            nexttok(ti);
            p_enumlist(ti, ident, flow);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACE) {
                nexttok(ti);
            }
            else {
                ERR("Syntax Error: Expected }");
                synerr_rec(ti);
            }
        }
        else {
            ERR("Syntax Error: Expected {");
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_STRUCTTYPE) {
        nexttok(ti);
        //struct <optid> { <structdeclarator> }
        p_optid(ti, dec);
        t = tok(ti);
        if(t->type == TOKTYPE_OPENBRACE) {
            nexttok(ti);
            p_structdeclarator(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACE) {
                nexttok(ti);
            }
            else {
                ERR("Syntax Error: Expected }");
                synerr_rec(ti);
            }
        }
        else {
            ERR("Syntax Error: Expected {");
            synerr_rec(ti);
        }
    }
    else {
        //syntax error
        ERR("Syntax Error: Expected var");
        synerr_rec(ti);
    }
    return dec;
}

void p_enumlist(tokiter_s *ti, node_s *root, flow_s *flow)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        nexttok(ti);
        p_assign(ti, root, flow);
        p_enumlist_(ti, root, flow);
    }
}

void p_enumlist_(tokiter_s *ti, node_s *root, flow_s *flow)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            p_assign(ti, root, flow);
            p_enumlist_(ti, root, flow);
        }
        else {
            ERR("Syntax Error: Expected identifier");
            synerr_rec(ti);
        }
    }
}

void p_optid(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        nexttok(ti);
        
    }
}

void p_structdeclarator(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_COLON) {
            nexttok(ti);
            p_type(ti, NULL);
            t = tok(ti);
            if(t->type == TOKTYPE_COMMA) {
                nexttok(ti);
                p_structdeclarator(ti);
            }
        }
        else {
            ERR("Syntax Error: Expected : but got: ");
            synerr_rec(ti);
        }
    }
    else {
        //epsilon production
    }
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
            type->ntype = TYPE_TYPEEXP;
            type->tok= t;
            break;
        case TOKTYPE_INTEGER:
            nexttok(ti);
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_CHARTYPE:
            nexttok(ti);
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_REAL:
            nexttok(ti);
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_STRINGTYPE:
            nexttok(ti);
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_REGEXTYPE:
            nexttok(ti);
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_IDENT:
            nexttok(ti);
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            p_array(ti, type, flow);
            break;
        case TOKTYPE_MAPTYPE:
            nexttok(ti);
            type = MAKENODE();
            type->ntype = TYPE_NODE;
            type->tok = t;
            p_array(ti, type, flow);
            t = tok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                p_type(ti, flow);
                t = tok(ti);
                if(t->type == TOKTYPE_DMAP) {
                    nexttok(ti);
                    p_type(ti, flow);
                    t = tok(ti);
                    if(t->type == TOKTYPE_CLOSEBRACE) {
                        nexttok(ti);
                    }
                    else {
                        ERR("Syntax Error: Expected } but got:");
                        synerr_rec(ti);
                    }
                }
                else {
                    ERR("Syntax Error: Expected => but got:");
                    synerr_rec(ti);
                }
            }
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            opt = p_typelist(ti, flow);
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
                    ERR("Syntax Error: Expected ->");
                    synerr_rec(ti);
                }
            }
            else {
                type = NULL;
                //syntax error
                ERR("Syntax Error: Expected )");
                synerr_rec(ti);
            }
            break;
        default:
            type = NULL;
            //syntax error
            ERR("Syntax Error: Expected void, int, real, string, regex, set, or (");
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
        op->ntype = TYPE_OP;
        op->tok = t;
        
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            ident = MAKENODE();
            ident->ntype = TYPE_IDENT;
            ident->tok = t;
            addchild(root, op);
            addchild(root, ident);
        }
        else {
            ERR("Syntax Error: Expected identifier");
            synerr_rec(ti);
        }
    }
}

void p_declist(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *dec;
    
    while(t->type == TOKTYPE_VAR || t->type == TOKTYPE_CLASS || t->type == TOKTYPE_ENUM || t->type == TOKTYPE_LET) {
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
            ERR("Syntax Error: Expected ]");
            synerr_rec(ti);
        }
    }
}

node_s *p_typelist(tokiter_s *ti, flow_s *flow)
{
    node_s *type;
    tok_s *t = tok(ti);
    node_s *root = MAKENODE();
    root->ntype = TYPE_NODE;
    
    switch(t->type) {
        case TOKTYPE_VOID:
        case TOKTYPE_INTEGER:
        case TOKTYPE_REAL:
        case TOKTYPE_STRINGTYPE:
        case TOKTYPE_REGEXTYPE:
        case TOKTYPE_OPENPAREN:
            type = p_type(ti, flow);
            addchild(root, type);
            p_typelist_(ti, root, flow);
            break;
        case TOKTYPE_IDENT:
            t = peeknexttok(ti);
            if(t->type == TOKTYPE_COLON) {
                nexttok(ti);
                nexttok(ti);
            }
            type = p_type(ti, flow);
            addchild(root, type);
            p_typelist_(ti, root, flow);
            break;
        case TOKTYPE_VARARG:
            nexttok(ti);
            break;
        default:
            //epsilon production
            break;
    }
    
    return root;
}

void p_typelist_(tokiter_s *ti, node_s *root, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *type;
    
    if(t->type == TOKTYPE_COMMA) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_VARARG) {
            type = MAKENODE();
            type->ntype = TYPE_TYPEEXP;
            type->tok = t;
            addchild(root, type);
            nexttok(ti);
        }
        else {
            if(t->type == TOKTYPE_IDENT) {
                t = peeknexttok(ti);
                if(t->type == TOKTYPE_COLON) {
                    nexttok(ti);
                    nexttok(ti);
                }
            }
            type = p_type(ti, flow);
            addchild(root, type);
            p_typelist_(ti, root, flow);
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
        n->ntype = TYPE_NODE;

        ident = MAKENODE();
        ident->ntype = TYPE_IDENT;
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
        ERR("Syntax Error: Expected identifier");
        synerr_rec(ti);
        return NULL;
    }
}

node_s *p_optparamlist(tokiter_s *ti, flow_s *flow)
{
    node_s *empty;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT || t->type == TOKTYPE_VARARG) {
        return p_paramlist(ti, flow);
    }
    else {
        //epsilon production
        empty = MAKENODE();
        empty->ntype = TYPE_NODE;
        return empty;
    }
}

node_s *p_paramlist(tokiter_s *ti, flow_s *flow)
{
    tok_s *t = tok(ti);
    node_s *root = MAKENODE(), *dec;

    root->ntype = TYPE_PARAMLIST;
    
    if(t->type == TOKTYPE_IDENT) {
        dec = p_param(ti, flow);
        addchild(root, dec);
        p_paramlist_(ti, root, flow);
    }
    else if(t->type == TOKTYPE_VARARG){
        nexttok(ti);
    }
    else {
        //syntax error
        ERR("Syntax Error: Expected identifier or ...");
        synerr_rec(ti);
    }
    
    return root;
}

void p_paramlist_(tokiter_s *ti, node_s *root, flow_s *flow)
{
    node_s *dec;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        t = nexttok(ti);
        
        if(t->type == TOKTYPE_IDENT) {
            dec = p_param(ti, flow);
        
            addchild(root, dec);
            p_paramlist_(ti, root, flow);
        }
        else if(t->type == TOKTYPE_VARARG) {
            dec = MAKENODE();
            dec->ntype = TYPE_DEC;
            dec->tok = t;
            nexttok(ti);
            addchild(root, dec);
        }
        else {
            //syntax error
            ERR("Syntax Error: Expected identifier or ...");
            synerr_rec(ti);
        }
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
    e->branch_complete = true;
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
        if(assign->ntype == TYPE_OP) {
            
        }
        else {
            opttype = assign;
            assign = right(opttype);
            if(assign) {
                
            }
        }
    }
}

edge_s *edge_s_(node_s *stmt)
{
    edge_s *e = alloc(sizeof *e);
    e->value = stmt;
    return e;
}

flow_s *flow_s_(void)
{
    flow_s *f = alloc(sizeof *f);
    
    f->start = allocz(sizeof *f);
    f->curr = f->start;
    f->final = NULL;
    return f;
}

flnode_s *flnode_s_(void)
{
    return allocz(sizeof(flnode_s));
}

void addflow(flnode_s *out, node_s *stmt, flnode_s *in)
{
    edge_s *e;
    
    if(out && in) {
        e = edge_s_(stmt);
        e->in = in;
        e->out = out;
        
        out->out = ralloc(out->out, (out->nout + 1) * sizeof(*out->out));
        out->out[out->nout] = e;
        out->nout++;
        
        in->in = ralloc(in->in, (in->nin + 1) * sizeof(*in->in));
        in->in[in->nin] = e;
        in->nin++;
    }
}

void flow_reparent(flnode_s *f1, flnode_s *f2)
{
    unsigned i;
    
    for(i = 0; i < f1->nin; i++) {
        f2->in = ralloc(f2->in, (f2->nin + 1) * sizeof(*f2->in));
        f2->in[f2->nin] = f1->in[i];
        f2->nin++;
    }
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
    
    if(root->ntype == TYPE_OP) {
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

void emit(tokiter_s *ti, char *str, ...)
{
    char *s;
    buf_s *b = ti->code;
    va_list argp;
    
    va_start(argp, str);
    
    bufaddstr(b, str, strlen(str));

    while((s = va_arg(argp, char *))) {
        bufaddstr(b, s, strlen(s));
    }
    va_end(argp);
}

void makelabel(tokiter_s *ti, char *buf)
{
    sprintf(buf, "_l%u:\n", ti->labelcount);
    ti->labelcount++;
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

void adderr(tokiter_s *ti, char *err, int lineno)
{
    char *s;
    errlist_s *e;
    tok_s *t;
    short ndig;
    
    if(lineno <= 0) {
        t = tok(ti);
        ndig = ndigits(t->line);
        if(lineno) {
            s = alloc(strlen(err) + sizeof(" but got") + strlen(t->lex) + sizeof(" at line: ") + ndig);
            sprintf(s, "%s but got %s at line: %d.", err, t->lex, t->line);
        }
        else {
            s = alloc(strlen(err) + sizeof(" at line: .") + ndig);
            sprintf(s, "%s at line: %d.", err, t->line);
        }
        
    }
    else {
        ndig = ndigits(lineno);
        s = alloc(strlen(err) + sizeof(" at line: .") + ndig);
        sprintf(s, "%s at line: %d.", err, lineno);
    }
    asm("hlt");
    
    e = alloc(sizeof(*e));
    e->next = NULL;
    e->msg = s;
    if(ti->err)
        ti->ecurr->next = e;
    else
        ti->err = e;
    ti->ecurr = e;
}
