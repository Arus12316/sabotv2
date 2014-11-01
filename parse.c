/*
 <statementlist> -> <statement> <statementlist> | ε
 
 <statement> ->  <expression> | <control> | <dec> | return <optexpression> | ; | continue | break

 <optexpression> -> <expression> | ε
 
 
 <expression> -> <shiftexpr> <expression'>
 
 <expression'> -> relop <shiftexpr> | ε

 
 <shiftexpr> -> <simple_expression> <shiftexpr'>
 
 <shiftexpr'> -> shiftop <simple_expression> | ε
 
 
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
                [ <arrayinit> ]
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
 
 <arrayinit> -> <expression> , <arrayinit> | ε
 
 <structliteral> -> ${ <initializerlist> }
 
 <initializerlist> -> <expression> , <initializerlist> | dot id assignop <expression> , <initializerlist> | ε
 
 <mapliteral> -> #{ <maplist> }
 
 <maplist> -> <expression> => <expression> <maplist> | ε
 
 
 <lambda> -> @(<optparamlist>) -> <type> openbrace <statementlist> closebrace
 
 <sign> -> + | -
 
 <optarglist> -> <arglist> | ε
 <arglist> -> <expression> <arglist'>
 <arglist'> -> , <expression> <arglist'> | ε
 
 <control> ->	while <expression> <controlsuffix>
                |
                for id <- <expression> <controlsuffix>
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
 
 <dec> -> <varlet> id <opttype> <assign> | class id <inh> { <declist> } | enum <optid> { <enumlist> } | struct id { <structdeclarator> }
 
 <varlet> -> var | let
 
 <enumlist> -> id <assign> <enumlist'> | ε
 <enumlist'> -> , id <assign> <enumlist'> | ε
 
 <optid> -> id | ε
 
 <structdeclarator> -> id : type , <structdeclarator> | ε
 
 <opttype> -> : <type> | ε
 
 <type> -> void | int <array> <mapdec> | char <array> <mapdec> | real <array> <mapdec> | string <array> <mapdec> |
            regex <array> <mapdec> |
            id <array> <mapdec> | ( <typelist> ) <array> <mapdec> map <type>
 
 <mapdec> -> => <type>
 
 <inh> -> : id | ε
 <declist> ->  <optsign> <dec> <optsemicolon> <declist> | <destructannotation> _(<paramlist>) {<statementlist>} <declist> 
                | { <op> } (<optparamlist>) { <statementlist> } <declist> |ε
 
 <destructannotation> -> ~ | ε
 
 <optsign> -> <sign> | ε

 <optsemicolon> -> ; | ε
 
 <array> -> [ <optexpression> ] <array> | ε
 
 <typelist> -> <optname> <type> <typelist'> | vararg <typelist'> | ε
 <typelist'> -> , <optname> <type> <typelist'> | vararg | ε
 <optname> -> id : | ε
 
 <optparamlist> -> <paramlist> | ε
 <param> -> id <opttype> <assign>
 <paramlist> -> <param> <paramlist'> | vararg
 <paramlist'> -> , <param> <paramlist'> | vararg | ε
 
 */

#include "parse.h"
#include "tree.h"
#include "general.h"
#include "types.h"
#include <stdio.h>
#include <stdint.h>
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

typedef struct tokchunk_s tokchunk_s;
typedef struct tokiter_s tokiter_s;

static tok_s eoftok = {
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
    bool firstpass;
    tokchunk_s *curr;
    errlist_s *err;
    errlist_s *ecurr;
    scope_s *scope;
    buf_s *code;
    unsigned labelcount;
};

static struct keyw_s {
    char *str;
    uint8_t type;
}
keywords[] = {
    {"if", TOKTYPE_IF},
    {"else", TOKTYPE_ELSE},
    {"while", TOKTYPE_WHILE},
    {"for", TOKTYPE_FOR},
    {"switch", TOKTYPE_SWITCH},
    {"case", TOKTYPE_CASE},
    {"default", TOKTYPE_DEFAULT},
    {"var", TOKTYPE_VAR},
    {"let", TOKTYPE_LET},
    {"return", TOKTYPE_RETURN},
    {"class", TOKTYPE_CLASS},
    {"do", TOKTYPE_DO},
    {"break", TOKTYPE_BREAK},
    {"continue", TOKTYPE_CONTINUE},
    {"enum", TOKTYPE_ENUM},
    {"struct", TOKTYPE_STRUCTTYPE}
};


static tokiter_s *lex(char *src);
static tok_s *mtok(tokchunk_s **list, uint16_t line, char *lexeme, size_t len, uint8_t type, uint8_t att);
static bool trykeyword(tokchunk_s **list, tok_s **prev, uint16_t line, char *str);
static void printtoks(tokchunk_s *list);

static tok_s *tok(tokiter_s *ti);
static tok_s *nexttok(tokiter_s *ti);
static tok_s *peeknexttok(tokiter_s *ti);

static node_s *start(tokiter_s *ti);
static void p_statementlist(tokiter_s *ti, node_s *root, node_s *last);
static node_s *p_statement(tokiter_s *ti);
static node_s *p_optexpression(tokiter_s *ti);
static node_s *p_expression(tokiter_s *ti);
static void p_expression_(tokiter_s *ti, node_s **p);
static node_s *p_shiftexpr(tokiter_s *ti);
static void p_shiftexpr_(tokiter_s *ti, node_s **p);

static node_s *p_simple_expression(tokiter_s *ti);
static void p_simple_expression_(tokiter_s *ti, node_s **p);
static node_s *p_term(tokiter_s *ti);
static void p_term_(tokiter_s *ti, node_s **p);
static node_s *p_factor(tokiter_s *ti);
static void p_optexp(tokiter_s *ti, node_s **p);
static node_s *p_factor_(tokiter_s *ti);
static void p_factor__(tokiter_s *ti, node_s **p);
static void p_assign(tokiter_s *ti, node_s **p);
static void p_arrayinit(tokiter_s *ti, node_s *root);
static node_s *p_structliteral(tokiter_s *ti);
static void p_initializerlist(tokiter_s *ti, node_s *root);
static node_s *p_mapliteral(tokiter_s *ti);
static void p_maplist(tokiter_s *ti, node_s *root);
static node_s *p_lambda(tokiter_s *ti);
static int p_sign(tokiter_s *ti);
static node_s *p_optarglist(tokiter_s *ti);
static node_s *p_arglist(tokiter_s *ti);
static void p_arglist_(tokiter_s *ti, node_s *root);
static node_s *p_control(tokiter_s *ti);
static node_s *p_if(tokiter_s *ti);
static void p_else(tokiter_s *ti, node_s *root);
static node_s *p_switch(tokiter_s *ti);
static void p_caselist(tokiter_s *ti, node_s *root);
static void p_controlsuffix(tokiter_s *ti, node_s *root);
static node_s *p_dec(tokiter_s *ti);
static void p_enumlist(tokiter_s *ti, node_s *root);
static void p_enumlist_(tokiter_s *ti, node_s *root);
static char *p_optid(tokiter_s *ti);
static void p_structdeclarator(tokiter_s *ti, node_s *root);
static node_s *p_opttype(tokiter_s *ti);
static node_s *p_type(tokiter_s *ti);
static void p_inh(tokiter_s *ti, node_s *root);
static void p_declist(tokiter_s *ti, node_s *root);
static node_s *p_array(tokiter_s *ti, node_s *root);
static void p_mapdec(tokiter_s *ti, node_s **dec);

static node_s *p_typelist(tokiter_s *ti);
static void p_typelist_(tokiter_s *ti, node_s *root);

static node_s *p_optparamlist(tokiter_s *ti);
static node_s *p_paramlist(tokiter_s *ti);
static void p_paramlist_(tokiter_s *ti, node_s *root);
static void synerr_rec(tokiter_s *ti);

static node_s *node_s_(scope_s *scope);
static void addchild(node_s *root, node_s *c);
static node_s *right(node_s *node);
static node_s *left(node_s *node);

static unsigned pjwhash(char *key);
static bool addtype(scope_s *child, char *key, node_s *type);
static node_s *tablelookup(rec_s *table[], char *key);
static scope_s *idtinit(scope_s *parent);
static void pushscope(tokiter_s *ti);
static inline void popscope(tokiter_s *ti);
static bool checkflow(node_s *root);
static node_s *getparentfunc(node_s *start);
static node_s *getparentloop(node_s *start);
static node_s *typecmp(node_s *t1, node_s *t2);

static void freetree(node_s *root);


static void adderr(tokiter_s *ti, char *str, int lineno);

static void print_node(node_s *node);

errlist_s *parse(char *src)
{
    node_s *tree;
    tokiter_s *ti;
    
    ti = lex(src);
    
    tree = start(ti);
    
    walk_tree(ti->scope, tree);
    
    printf("code: %s\n", ti->code->buf);
    
    return ti->err;
}

tokiter_s *lex(char *src)
{
    int i;
    uint16_t line = 1;
    char *bptr, c;
    tok_s *prev = NULL;
    tokchunk_s *head, *curr;
    tokiter_s *ti;
    type_s **it;
    
    ti = alloc(sizeof *ti);
    curr = head = alloc(sizeof *head);
    
    head->size = 0;
    head->next = NULL;
    
    ti->err = NULL;
    ti->ecurr = NULL;
    ti->i = 0;
    ti->firstpass = true;
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
                if(*(src + 1) == '.') {
                    if(*(src + 2) == '.') {
                        prev = mtok(&curr, line, "...", 1, TOKTYPE_VARARG, TOKATT_DEFAULT);
                        src += 3;
                    }
                    else {
                        prev = mtok(&curr, line, "..", 1, TOKTYPE_RANGE, TOKATT_DEFAULT);
                        src += 2;
                    }
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
                if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "!=", 2, TOKTYPE_RELOP, TOKATT_NEQ);
                    src += 2;
                }
                else {
                    prev = mtok(&curr, line, "!", 1, TOKTYPE_NOT, TOKATT_DEFAULT);
                    src++;
                }
                break;
            case '&':
                prev = mtok(&curr, line, "&", 1, TOKTYPE_MULOP, TOKATT_AND);
                src++;
                break;
            case '|':
                prev = mtok(&curr, line, "|", 1, TOKTYPE_ADDOP, TOKATT_OR);
                src++;
                break;
            case '~':
                prev = mtok(&curr, line, "~", 1, TOKTYPE_COMPLEMENT, TOKATT_DEFAULT);
                src++;
                break;
            case '$':
                if(*(src + 1) == '{') {
                    prev = mtok(&curr, line, "${", 1, TOKTYPE_STRUCTLITERAL, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    adderr(ti, "Lexical Error: Expected '{' for '${", line);
                    src++;
                }
                break;
            case '#':
                if(*(src + 1) == '{') {
                    prev = mtok(&curr, line, "!", 1, TOKTYPE_MAPLITERAL, TOKATT_DEFAULT);
                    src += 2;
                }
                else {
                    adderr(ti, "Lexical Error: Expected '{' for '${", line);
                    src++;
                }
                break;
            case '<':
                if(*(src + 1) == '-') {
                    prev = mtok(&curr, line, "<-", 2, TOKTYPE_FORVAR, TOKATT_DEFAULT);
                    src += 2;
                }
                else if(*(src + 1) == '=') {
                    prev = mtok(&curr, line, "<=", 2, TOKTYPE_RELOP, TOKATT_LEQ);
                    src += 2;
                }
                else if(*(src + 1) == '<') {
                    if(*(src + 2) == '*') {
                        prev = mtok(&curr, line, "<<*", 3, TOKTYPE_SHIFT, TOKATT_LCSHIFT);
                        src += 3;
                    }
                    else {
                        prev = mtok(&curr, line, "<<", 2, TOKTYPE_SHIFT, TOKATT_LSHIFT);
                        src += 2;
                    }
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
                else if(*(src + 1) == '>') {
                    if(*(src + 2) == '*') {
                        prev = mtok(&curr, line, ">>*", 3, TOKTYPE_SHIFT, TOKATT_RCSHIFT);
                        src += 3;
                    }
                    else {
                        prev = mtok(&curr, line, ">>", 2, TOKTYPE_SHIFT, TOKATT_RSHIFT);
                        src += 2;
                    }
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
                            if(*src == '\\') {
                                src++;
                            }
                            if(*src) {
                                src++;
                            }
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
    
    return &l->tok[size];
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
    int i;
    primtype_s *type;
    tok_s *t;
    node_s *root = MAKENODE();
    
    root->type = TOKTYPE_STMTLIST;
    
    ti->scope = idtinit(NULL);
    ti->code = bufinit();
    ti->labelcount = 0;
    
    for(i = 0; (type = nextype(i)); i++) {
        addident(ti->scope->types, type->str, NULL);
    }

    p_statementlist(ti, root, NULL);
    t = tok(ti);
    
    if(t->type != TOKTYPE_EOF) {
        //syntax error
        ERR("Syntax Error: Expected EOF");
    }
    
    return root;
}

void p_statementlist(tokiter_s *ti, node_s *root, node_s *last)
{
    node_s *statement;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_CLASS:
        case TOKTYPE_VAR:
        case TOKTYPE_LET:
        case TOKTYPE_ENUM:
        case TOKTYPE_SWITCH:
        case TOKTYPE_FOR:
        case TOKTYPE_WHILE:
        case TOKTYPE_DO:
        case TOKTYPE_IF:
        case TOKTYPE_ADDOP:
        case TOKTYPE_OPENBRACKET:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_REGEX:
        case TOKTYPE_CHAR:
        case TOKTYPE_STRING:
        case TOKTYPE_NOT:
        case TOKTYPE_COMPLEMENT:
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
            statement = p_statement(ti);
            addchild(root, statement);
            p_statementlist(ti, root, statement);
            break;
        case TOKTYPE_CLOSEBRACE:
        case TOKTYPE_EOF:
            //epsilon production
            break;
        default:
            ERR("Syntax Error: Expected class, var, listener, switch, for, while, if, +, -, {, }, ^{, \
                regex, string, !, (, number, identifier, or return");
            synerr_rec(ti);
            break;
    }
}

node_s *p_statement(tokiter_s *ti)
{
    node_s *statement, *exp;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
        case TOKTYPE_NUM:
        case TOKTYPE_DOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NOT:
        case TOKTYPE_COMPLEMENT:
        case TOKTYPE_IF:
        case TOKTYPE_SWITCH:
        case TOKTYPE_CHAR:
        case TOKTYPE_STRING:
        case TOKTYPE_REGEX:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_OPENBRACKET:
        case TOKTYPE_STRUCTLITERAL:
        case TOKTYPE_MAPLITERAL:
        case TOKTYPE_ADDOP:
            statement = p_expression(ti);
            return statement;
        case TOKTYPE_WHILE:
        case TOKTYPE_DO:
        case TOKTYPE_FOR:
            statement =  p_control(ti);
            statement->branch_complete = false;
            return statement;
        case TOKTYPE_VAR:
        case TOKTYPE_LET:
        case TOKTYPE_CLASS:
        case TOKTYPE_ENUM:
        case TOKTYPE_STRUCTTYPE:
            return p_dec(ti);
        case TOKTYPE_RETURN:
            nexttok(ti);
            statement = MAKENODE();
            statement->type = TOKTYPE_RETURN;
            exp = p_optexpression(ti);
            addchild(statement, exp);
            return statement;
        case TOKTYPE_SEMICOLON:
            nexttok(ti);
            statement = MAKENODE();
            statement->type = TOKTYPE_STMTVOID;
            statement->att = TOKATT_DEFAULT;
            return statement;
        case TOKTYPE_BREAK:
            nexttok(ti);
            statement = MAKENODE();
            statement->type = TOKTYPE_BREAK;
            statement->att = TOKATT_DEFAULT;
            addchild(statement, statement);
            return statement;
        case TOKTYPE_CONTINUE:
            nexttok(ti);
            statement = MAKENODE();
            statement->type = TOKTYPE_CONTINUE;
            statement->att = TOKATT_DEFAULT;
            addchild(statement, statement);
            return statement;
        default:
            //syntax error
            ERR("Syntax Error: Expected identifier, number, (, not, string, regex, @, {, +, -, !, if, while \
                for, switch, var, or return");
            synerr_rec(ti);
            return NULL;
    }
}

node_s *p_optexpression(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
        case TOKTYPE_NUM:
        case TOKTYPE_DOT:
        case TOKTYPE_OPENPAREN:
        case TOKTYPE_NOT:
        case TOKTYPE_COMPLEMENT:
        case TOKTYPE_IF:
        case TOKTYPE_SWITCH:
        case TOKTYPE_CHAR:
        case TOKTYPE_STRING:
        case TOKTYPE_REGEX:
        case TOKTYPE_LAMBDA:
        case TOKTYPE_OPENBRACKET:
        case TOKTYPE_STRUCTLITERAL:
        case TOKTYPE_MAPLITERAL:
        case TOKTYPE_ADDOP:
            return p_expression(ti);
        default:
            //epsilon production
            return NULL;
    }
}

node_s *p_expression(tokiter_s *ti)
{
    node_s *node, *exp;
    
    exp = MAKENODE();
    exp->type = TOKTYPE_EXPRESSION;
    
    node = p_shiftexpr(ti);
    p_expression_(ti, &node);
    addchild(exp, node);
    return exp;
}

void p_expression_(tokiter_s *ti, node_s **p)
{
    node_s *s = NULL, *op = NULL;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_RELOP) {
        nexttok(ti);
        s = p_shiftexpr(ti);
        
        op = MAKENODE();
        op->type = TOKTYPE_RELOP;
        op->att = t->att;
        
        addchild(op, *p);
        addchild(op, s);
        *p = op;
    }
}

node_s *p_shiftexpr(tokiter_s *ti)
{
    node_s *sexp;
    
    sexp = p_simple_expression(ti);
    
    p_shiftexpr_(ti, &sexp);
    
    return sexp;
}

void p_shiftexpr_(tokiter_s *ti, node_s **p)
{
    node_s *op, *sexp;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_SHIFT) {
        nexttok(ti);
        sexp = p_simple_expression(ti);
        
        op = MAKENODE();
        op->type = TOKTYPE_SHIFT;
        op->att = t->att;
        
        addchild(op, *p);
        addchild(op, sexp);
        *p = op;
    }
}

node_s *p_simple_expression(tokiter_s *ti)
{
    int sign = 0;
    node_s *term, *un = NULL, *p;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ADDOP)
        sign = p_sign(ti);
    term = p_term(ti);
    
    if(sign == -1) {
        un = MAKENODE();
        un->type = TOKTYPE_NEGATE;
        un->att = TOKATT_DEFAULT;
        p = un;
    }
    else {
        p = term;
    }
    p_simple_expression_(ti, &p);
    
    return p;
}

void p_simple_expression_(tokiter_s *ti, node_s **p)
{
    node_s *term = NULL, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ADDOP) {
        nexttok(ti);
        term = p_term(ti);
        
        op = MAKENODE();
        op->type = TOKTYPE_ADDOP;
        op->att = t->att;
        op->tok = t;
        
        p_simple_expression_(ti, &term);
        
        addchild(op, *p);
        addchild(op, term);
        *p = op;
    }
}

node_s *p_term(tokiter_s *ti)
{
    node_s *f;
    
    f = p_factor(ti);
    
    p_term_(ti, &f);
    
    return f;
}

void p_term_(tokiter_s *ti, node_s **p)
{
    node_s *f = NULL, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_MULOP) {
        nexttok(ti);
        f = p_factor(ti);

        op = MAKENODE();
        op->type = TOKTYPE_ADDOP;
        op->att = t->att;
        op->tok = t;
        
        p_term_(ti, &f);
        
        addchild(op, *p);
        addchild(op, f);
        *p = op;
    }
}

node_s *p_factor(tokiter_s *ti)
{
    node_s *f_;
    
    f_ = p_factor_(ti);
    
    p_optexp(ti, &f_);
    
    return f_;
}

void p_optexp(tokiter_s *ti, node_s **p)
{
    tok_s *t = tok(ti);
    node_s  *f, *op;
    
    if(t->type == TOKTYPE_EXPOP) {
        nexttok(ti);
        f = p_factor(ti);
        
        op = MAKENODE();
        op->type = TOKTYPE_EXPOP;
        op->att = TOKATT_DEFAULT;
        op->tok = t;
        
        addchild(op, *p);
        addchild(op, f);
        *p = op;
    }
}

node_s *p_factor_(tokiter_s *ti)
{
    node_s *n, *ident, *f, *op;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
            nexttok(ti);
            
            n = MAKENODE();
            n->type = TOKTYPE_IDENT;
            n->tok = t;
            
            p_factor__(ti, &n);
            p_assign(ti, &n);
            
            break;
        case TOKTYPE_DOT:
            op = MAKENODE();
            op->type = TOKTYPE_DOT;
            op->att = TOKATT_DEFAULT;
            op->tok = t;
            t = nexttok(ti);

            if(t->type == TOKTYPE_IDENT) {
                nexttok(ti);
                ident = MAKENODE();
                ident->type = TOKTYPE_IDENT;
                ident->tok = t;
                addchild(op, ident);
                p_factor__(ti, &op);
                n = op;
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
            n->type = TOKTYPE_NUM;
            n->att = t->att;
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
                ERR("Syntax Error: Expected )");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_NOT:
            nexttok(ti);
            op = MAKENODE();
            op->type = TOKTYPE_NOT;
            op->tok = t;
            f = p_factor(ti);
            addchild(op, f);
            n = op;
            break;
        case TOKTYPE_COMPLEMENT:
            nexttok(ti);
            op = MAKENODE();
            op->type = TOKTYPE_COMPLEMENT;
            op->tok = t;
            f = p_factor(ti);
            addchild(op, f);
            n = op;
            break;
        case TOKTYPE_CHAR:
            nexttok(ti);
            n = MAKENODE();
            n->type = TOKTYPE_CHAR;
            n->att = TOKATT_DEFAULT;
            n->tok = t;
            break;
        case TOKTYPE_STRING:
            nexttok(ti);
            n = MAKENODE();
            n->type = TOKTYPE_STRING;
            n->att = TOKATT_DEFAULT;
            n->tok = t;
            break;
        case TOKTYPE_REGEX:
            nexttok(ti);
            n = MAKENODE();
            n->type = TOKTYPE_REGEX;
            n->att = TOKATT_DEFAULT;
            n->tok = t;
            break;
        case TOKTYPE_OPENBRACKET:
            n = MAKENODE();
            n->type = TOKTYPE_ARRAYLIT;
            n->att = TOKATT_DEFAULT;
            n->tok = t;
            nexttok(ti);
            p_arrayinit(ti, n);
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACKET) {
                nexttok(ti);
            }
            else {
                ERR("Syntax Error: Expected ] but got");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_STRUCTLITERAL:
            n = p_structliteral(ti);
            break;
        case TOKTYPE_MAPLITERAL:
            return p_mapliteral(ti);
            break;
        case TOKTYPE_LAMBDA:
            n = p_lambda(ti);
            p_factor__(ti, &n);
            break;
        case TOKTYPE_IF:
            n = p_if(ti);
            break;
        case TOKTYPE_SWITCH:
            n = p_switch(ti);
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

void p_factor__(tokiter_s *ti, node_s **p)
{
    node_s *op, *exp, *ident, *opt;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_OPENBRACKET:
            nexttok(ti);
            exp = p_expression(ti);
            
            op = MAKENODE();
            op->type = TOKTYPE_SUBSCRIPT;
            op->tok = t;
            
            addchild(op, *p);
            addchild(op, exp);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEBRACKET) {
                nexttok(ti);
                p_factor__(ti, &op);
                *p = op;
            }
            else {
                //syntax error
                ERR("Syntax Error: Expected ]");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_DOT:
            op = MAKENODE();
            op->type = TOKTYPE_DOT;
            op->tok = t;

            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                nexttok(ti);
                ident = MAKENODE();
                ident->type = TOKTYPE_IDENT;
                ident->tok = t;
                addchild(op, *p);
                addchild(op, ident);
                p_factor__(ti, &op);
                *p = op;
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
            op->type = TOKTYPE_CALL;
            op->att = TOKATT_DEFAULT;
            op->tok = t;
            
            opt = p_optarglist(ti);
            addchild(op, *p);
            addchild(op, opt);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
                p_factor__(ti, &op);
                *p = op;
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

void p_assign(tokiter_s *ti, node_s **p)
{
    node_s *exp, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_ASSIGN) {
        nexttok(ti);
        exp = p_expression(ti);
        
        op = MAKENODE();
        op->type = TOKTYPE_ASSIGN;
        op->att = t->att;
        
        addchild(op, *p);
        addchild(op, exp);
        *p = op;
    }
}

void p_arrayinit(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *exp;
    
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
        case TOKTYPE_OPENBRACKET:
            exp = p_expression(ti);
            
            addchild(root, exp);
            t = tok(ti);
            if(t->type == TOKTYPE_COMMA) {
                nexttok(ti);
            }
            p_arrayinit(ti, root);
            break;
        default:
            //epsilon production
            break;
    }
}

node_s *p_structliteral(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    node_s *root = MAKENODE();
    
    root->tok = t;
    root->type = TOKTYPE_STRUCTLITERAL;
    root->att = TOKATT_DEFAULT;
    
    if(t->type == TOKTYPE_STRUCTLITERAL) {
        nexttok(ti);
        p_initializerlist(ti, root);
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
    return root;
}

void p_initializerlist(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *op, *ident, *exp;
    
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
        case TOKTYPE_OPENBRACKET:
            exp = p_expression(ti);
            addchild(root, exp);
            t = tok(ti);
            if(t->type == TOKTYPE_COMMA) {
                nexttok(ti);
                p_initializerlist(ti, root);
            }
            break;
        case TOKTYPE_DOT:
            op = MAKENODE();
            op->type = TOKTYPE_DOT;
            op->att = TOKATT_DEFAULT;
            
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                ident = MAKENODE();
                ident->type = TOKTYPE_IDENT;
                ident->att = TOKATT_DEFAULT;
                ident->tok = t;
                t = nexttok(ti);
                if(t->type == TOKTYPE_ASSIGN) {
                    nexttok(ti);
                    exp = p_expression(ti);
                    addchild(op, ident);
                    addchild(op, exp);
                    t = tok(ti);
                    if(t->type == TOKTYPE_COMMA) {
                        nexttok(ti);
                        p_initializerlist(ti, root);
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

node_s *p_mapliteral(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    node_s *root = MAKENODE();
    
    root->type = TOKTYPE_MAPLITERAL;
    root->att = TOKATT_DEFAULT;
    root->tok = t;
    
    if(t->type == TOKTYPE_MAPLITERAL) {
        nexttok(ti);
        p_maplist(ti, root);
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
    return root;
}

void p_maplist(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *op, *key, *val;
    
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
        case TOKTYPE_OPENBRACKET:
            op = MAKENODE();
            op->type = TOKTYPE_DMAP;
            op->att = TOKATT_DEFAULT;
            op->tok = t;
            key = p_expression(ti);
            t = tok(ti);
            if(t->type == TOKTYPE_DMAP) {
                nexttok(ti);
                val = p_expression(ti);
                addchild(op, key);
                addchild(op, val);
                addchild(root, op);
                t = tok(ti);
                if(t->type == TOKTYPE_COMMA) {
                    nexttok(ti);
                    p_maplist(ti, root);
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
    tok_s *t = tok(ti);
    node_s *lambda = MAKENODE(), *body = MAKENODE(),
            *param, *map = NULL, *type = NULL;
    
    lambda->type = TOKTYPE_LAMBDA;
    lambda->att = TOKATT_DEFAULT;
    lambda->tok = t;
    
    body->type = TOKTYPE_STMTLIST;
    body->att = TOKATT_DEFAULT;
    body->tok = t;
    
    t = nexttok(ti);
    
    PUSHSCOPE();

    if(t->type == TOKTYPE_OPENPAREN) {
        nexttok(ti);
        param = p_optparamlist(ti);
        addchild(lambda, param);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEPAREN)
            t = nexttok(ti);
        else {
            lambda = NULL;
            //syntax error
            ERR("Syntax Error: Expected )");
            synerr_rec(ti);
        }
    }
    else {
        param = NULL;
    }
    
    if(t->type == TOKTYPE_MAP) {
        nexttok(ti);
        map = MAKENODE();
        map->type = TOKTYPE_MAP;
        map->att = TOKATT_DEFAULT;
        
        type = p_type(ti);
        t = tok(ti);
    }
    
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        addchild(lambda, param);
        addchild(lambda, map);
        addchild(lambda, type);
        addchild(lambda, body);
        p_statementlist(ti, body, NULL);
        
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
    POPSCOPE();
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

node_s *p_optarglist(tokiter_s *ti)
{
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
            return p_arglist(ti);
            break;
        default:
            return NULL;
    }
}

node_s *p_arglist(tokiter_s *ti)
{
    node_s *root = MAKENODE(), *n;
    
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
}

node_s *p_control(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    node_s  *op = MAKENODE(),
            *exp, *ident, *suffix = MAKENODE();
    
    switch(t->type) {
        case TOKTYPE_WHILE:
            nexttok(ti);
            
            op->type = TOKTYPE_WHILE;
            op->att = TOKATT_DEFAULT;
            op->tok = t;
            
            suffix->type = TOKTYPE_STMTLIST;
            suffix->att = TOKATT_DEFAULT;
            
            exp = p_expression(ti);
            addchild(op, exp);
            addchild(op, suffix);
            p_controlsuffix(ti, suffix);
            break;
        case TOKTYPE_FOR:
            t = nexttok(ti);
            if(t->type == TOKTYPE_IDENT) {
                
                op->type = TOKTYPE_FOR;
                op->att = TOKATT_DEFAULT;
                op->tok = t;

                ident = MAKENODE();
                ident->type = TOKTYPE_IDENT;
                ident->tok = t;
                t = nexttok(ti);
                if(t->type == TOKTYPE_FORVAR) {
                    nexttok(ti);
                    exp = p_expression(ti);
                    addchild(op, ident);
                    addchild(op, exp);
                    addchild(op, suffix);
                    p_controlsuffix(ti, suffix);
                }
                else {
                    //syntax error
                    ERR("Syntax Error: Expected <-");
                    synerr_rec(ti);
                }
            }
            else {
                //syntax error
                ERR("Syntax Error: Expected identifier");
                synerr_rec(ti);
            }
            break;
        case TOKTYPE_DO:
            nexttok(ti);

            op->type = TOKTYPE_DO;
            op->att = TOKATT_DEFAULT;
            op->tok = t;

            p_controlsuffix(ti, suffix);
            t = tok(ti);
            if(t->type == TOKTYPE_WHILE) {
                nexttok(ti);
                exp = p_expression(ti);
                addchild(op, suffix);
                addchild(op, exp);
            }
            else {
                //syntax error
                ERR("Syntax Error: Expected while");
                synerr_rec(ti);
            }
            break;
        default:
            //syntax error
            ERR("Syntax Error: Expected if, while, for, or switch");
            synerr_rec(ti);
            break;
    }
    return op;
}

node_s *p_if(tokiter_s *ti)
{
    node_s  *op = MAKENODE(),
            *exp, *suffix = MAKENODE();
    
    op->type = TOKTYPE_IF;
    op->att = TOKATT_DEFAULT;
    
    suffix->type = TOKTYPE_STMTLIST;
    suffix->att = TOKATT_DEFAULT;
    
    nexttok(ti);
    exp = p_expression(ti);
    
    addchild(op, exp);
    addchild(op, suffix);

    p_controlsuffix(ti, suffix);
    
    p_else(ti, op);
    
    return op;
}

bool checkflow(node_s *root)
{
    if(root->nchildren > 4) {
        return true;
    }
    return false;
}

void p_else(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *op;
    
    if(t->type == TOKTYPE_ELSE) {
        nexttok(ti);
        op = MAKENODE();
        op->type = TOKTYPE_STMTLIST;
        op->att = TOKATT_DEFAULT;
        op->tok = t;
        
        addchild(root, op);
        p_controlsuffix(ti, op);
    }
}

node_s *p_switch(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    node_s *op = MAKENODE(), *exp;
    
    op->type = TOKTYPE_SWITCH;
    op->att = TOKATT_DEFAULT;
    
    t = nexttok(ti);
    if(t->type == TOKTYPE_OPENPAREN) {
        nexttok(ti);
        exp = p_expression(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEPAREN) {
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {
                nexttok(ti);
                
                addchild(op, exp);
                
                p_caselist(ti, op);
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
    return op;
}


void p_caselist(tokiter_s *ti, node_s *root)
{
    node_s *exp, *arg, *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_CASE) {
        nexttok(ti);
        
        arg = p_arglist(ti);
        t = tok(ti);
        if(t->type == TOKTYPE_MAP) {
            nexttok(ti);
            
            op = MAKENODE();
            op->type = TOKTYPE_CASE;
            op->att = TOKATT_DEFAULT;
            op->tok = t;
            
            exp = p_expression(ti);
            addchild(op, arg);
            addchild(op, exp);
            addchild(root, op);
            p_caselist(ti, root);
        }
        else {
            //syntax error
            ERR("Syntax Error: Expected ->");
            synerr_rec(ti);
        }
    }
    else if(t->type == TOKTYPE_DEFAULT) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_MAP) {
            nexttok(ti);
            
            op = MAKENODE();
            op->type = TOKTYPE_DEFAULT;
            op->att = TOKATT_DEFAULT;
            op->tok = t;
            
            exp = p_expression(ti);
            addchild(op, exp);
            addchild(root, op);
            p_caselist(ti, root);
        }
        else {
            //syntax error
            ERR("Syntax Error: Expected ->");
            synerr_rec(ti);
        }
    }
}

void p_controlsuffix(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *statement, *last;
    
    if(t->type == TOKTYPE_OPENBRACE) {
        nexttok(ti);
        PUSHSCOPE();
        
        p_statementlist(ti, root, NULL);
        POPSCOPE();
        if(root->nchildren) {
            last = root->children[root->nchildren - 1];
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
        statement = p_statement(ti);
        addchild(root, statement);
        root->branch_complete = statement->branch_complete;
    }
}

node_s *p_dec(tokiter_s *ti)
{
    char *id;
    node_s *ident, *dectype, *dec;
    tok_s *t = tok(ti);
    
    dec = MAKENODE();
    dec->type = t->type;
    dec->att = TOKATT_DEFAULT;
    
    if(t->type == TOKTYPE_VAR || t->type == TOKTYPE_LET) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            
            ident = MAKENODE();
            ident->type = TOKTYPE_IDENT;
            ident->att = TOKATT_DEFAULT;
            ident->tok = t;
            
            addchild(dec, ident);
            
            dectype = p_opttype(ti);
            addchild(dec, dectype);
            p_assign(ti, &dec);
            
            if(!dectype) {
                if(dec->type != TOKTYPE_ASSIGN) {
                    ERR("Error: Declaration must either have a type annotation or an assignment");
                }
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
            ident->type = TOKTYPE_IDENT;
            ident->att = TOKATT_DEFAULT;
            ident->tok = t;
            
            addchild(dec, ident);

            if(addtype(ti->scope, t->lex, dec)) {
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
        ident->type = TOKTYPE_IDENT;
        ident->att = TOKATT_DEFAULT;
        ident->tok = t;
        
        id = p_optid(ti);
        if(id) {
            if(addtype(ti->scope, t->lex, dec)) {
                // adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
            }
        }
        t = tok(ti);
        
        if(t->type == TOKTYPE_OPENBRACE) {
            nexttok(ti);
            addchild(dec, ident);
            p_enumlist(ti, ident);
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
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            ident = MAKENODE();
            ident->type = TOKTYPE_IDENT;
            ident->att = TOKATT_DEFAULT;
            ident->tok = t;
            
            if(addtype(ti->scope, t->lex, dec)) {
                // adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
            }
            
            t = nexttok(ti);
            if(t->type == TOKTYPE_OPENBRACE) {

                nexttok(ti);
                p_structdeclarator(ti, dec);
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
            ERR("Syntax Error: Expected identifier");
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

void p_enumlist(tokiter_s *ti, node_s *root)
{
    node_s *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        op = MAKENODE();
        op->type = TOKTYPE_IDENT;
        op->att = TOKATT_DEFAULT;
        op->tok = t;
        
        nexttok(ti);
        p_assign(ti, &op);
        addchild(root, op);
        
        p_enumlist_(ti, root);
    }
}

void p_enumlist_(tokiter_s *ti, node_s *root)
{
    node_s *op;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            op = MAKENODE();
            op->type = TOKTYPE_IDENT;
            op->att = TOKATT_DEFAULT;
            op->tok = t;
            
            nexttok(ti);
            p_assign(ti, &op);
            addchild(root, op);
            
            p_enumlist_(ti, root);
        }
        else {
            ERR("Syntax Error: Expected identifier");
            synerr_rec(ti);
        }
    }
}

char *p_optid(tokiter_s *ti)
{
    tok_s *t = tok(ti), *bck;
    
    if(t->type == TOKTYPE_IDENT) {
        bck = t;
        nexttok(ti);
        return bck->lex;
    }
    return NULL;
}

void p_structdeclarator(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *ident, *type;
    
    if(t->type == TOKTYPE_IDENT) {
        ident = MAKENODE();
        ident->type = TOKTYPE_IDENT;
        ident->tok = t;
        t = nexttok(ti);
        if(t->type == TOKTYPE_COLON) {
            nexttok(ti);
            type = p_type(ti);
            addchild(root, ident);
            addchild(root, type);
            t = tok(ti);
            if(t->type == TOKTYPE_COMMA) {
                nexttok(ti);
                p_structdeclarator(ti, root);
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
    node_s *type, *array, *opt, *ret;
    tok_s *t = tok(ti);
    
    switch(t->type) {
        case TOKTYPE_IDENT:
            nexttok(ti);
            type = MAKENODE();
            type->type = TOKTYPE_IDENT;
            type->att = TOKATT_DEFAULT;
            type->tok = t;
            array = p_array(ti, NULL);
            addchild(type, array);
            p_mapdec(ti, &type);
            break;
        case TOKTYPE_OPENPAREN:
            nexttok(ti);
            type = MAKENODE();
            type->type = TOKTYPE_CLOSURE;
            type->att = TOKATT_DEFAULT;
            type->tok = t;
            opt = p_typelist(ti);
            addchild(type, opt);
            
            t = tok(ti);
            if(t->type == TOKTYPE_CLOSEPAREN) {
                nexttok(ti);
                array = p_array(ti, NULL);
                addchild(type, array);
                t = tok(ti);
                if(t->type == TOKTYPE_MAP) {
                    nexttok(ti);
                    ret = p_type(ti);
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
        op->type = TOKTYPE_COLON;
        op->att = TOKATT_DEFAULT;
        op->tok = t;
        
        t = nexttok(ti);
        if(t->type == TOKTYPE_IDENT) {
            nexttok(ti);
            ident = MAKENODE();
            ident->type = TOKTYPE_IDENT;
            ident->att = TOKATT_DEFAULT;
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
    int sign;
    tok_s *t = tok(ti);
    node_s *dec, *st, *dc = NULL, *opt, *stmt, *op;
    
    switch(t->type) {
        case TOKTYPE_LET:
        case TOKTYPE_VAR:
        case TOKTYPE_CLASS:
        case TOKTYPE_ENUM:
        case TOKTYPE_STRUCTTYPE:
            dec = p_dec(ti);
            addchild(root, dec);
            t = tok(ti);
            if(t->type == TOKTYPE_SEMICOLON)
                t = nexttok(ti);
            p_declist(ti, root);
            break;
        case TOKTYPE_OPENBRACE:
            t = nexttok(ti);
            dec = MAKENODE();
            dec->type = TOKTYPE_OVERLOADOP;
            dec->att = TOKATT_DEFAULT;
            switch(t->type) {
                case TOKTYPE_ADDOP:
                case TOKTYPE_MULOP:
                case TOKTYPE_SHIFT:
                case TOKTYPE_EXPOP:
                case TOKTYPE_NOT:
                case TOKTYPE_COMPLEMENT:
                case TOKTYPE_ASSIGN:
                    op = MAKENODE();
                    op->type = t->type;
                    op->att = t->att;
                    addchild(dec, op);
                    break;
                case TOKTYPE_OPENBRACKET:
                    t = nexttok(ti);
                    if(t->type != TOKTYPE_CLOSEBRACKET) {
                        ERR("Syntax Error: Expected ']");
                    }
                    op = MAKENODE();
                    op->type = TOKTYPE_OPENBRACKET;
                    op->att = TOKATT_DEFAULT;
                    addchild(op, dec);
                    break;
                default:
                    ERR("Operator Cannot Be Overloaded");
                    break;
            }
            t = nexttok(ti);
            if(t->type == TOKTYPE_CLOSEBRACE) {
                t = nexttok(ti);
                if(t->type == TOKTYPE_OPENPAREN) {
                    nexttok(ti);
                    opt = p_optparamlist(ti);
                    addchild(dec, opt);
                    t = tok(ti);
                    if(t->type == TOKTYPE_CLOSEPAREN) {
                        t = nexttok(ti);
                        if(t->type == TOKTYPE_OPENBRACE) {
                            nexttok(ti);
                            stmt = MAKENODE();
                            stmt->type = TOKTYPE_STMTLIST;
                            stmt->att = TOKATT_DEFAULT;
                            addchild(dec, stmt);
                            p_statementlist(ti, stmt, NULL);
                            t = tok(ti);
                            if(t->type == TOKTYPE_CLOSEBRACE) {
                                nexttok(ti);
                            }
                            else {
                                ERR("Syntax Error: Expected '}'");
                                synerr_rec(ti);
                            }
                        }
                        else {
                            ERR("Syntax Error: Expected '{");
                            synerr_rec(ti);
                        }
                    }
                    else {
                        ERR("Syntax Error: Expected ')");
                        synerr_rec(ti);
                    }
                }
                else {
                    ERR("Syntax Error: Expected '(");
                    synerr_rec(ti);
                }
            }
            else {
                ERR("Syntax Error: Expected '}");
                synerr_rec(ti);
            }
            addchild(root, dec);
            p_declist(ti, root);
            break;
        case TOKTYPE_ADDOP:
            sign = p_sign(ti);
            if(sign == 1) {
                st = MAKENODE();
                st->type = TOKTYPE_STATIC;
                st->att = TOKATT_DEFAULT;
                addchild(root, st);
            }
            dec = p_dec(ti);
            addchild(root, dec);
            t = tok(ti);
            if(t->type == TOKTYPE_SEMICOLON)
                t = nexttok(ti);
            p_declist(ti, root);
            break;
        case TOKTYPE_COMPLEMENT:
            dc = MAKENODE();
            dc->type = TOKTYPE_DESTRUCTOR;
            dc->att = TOKATT_DEFAULT;
            t = nexttok(ti);
        case TOKTYPE_IDENT:
            if(!strcmp(t->lex, "_")) {
                if(!dc) {
                    dc = MAKENODE();
                    dc->type = TOKTYPE_CONSTRUCTOR;
                    dc->att = TOKATT_DEFAULT;
                }
                t = nexttok(ti);
                if(t->type == TOKTYPE_OPENPAREN) {
                    t = nexttok(ti);
                    opt = p_optparamlist(ti);
                    addchild(root, dc);
                    addchild(dc, opt);
                    t = tok(ti);
                    if(t->type == TOKTYPE_CLOSEPAREN) {
                        t = nexttok(ti);
                        if(t->type == TOKTYPE_OPENBRACE) {
                            nexttok(ti);
                            
                            stmt = MAKENODE();
                            stmt->type = TOKTYPE_STMTLIST;
                            stmt->att = TOKATT_DEFAULT;
                            
                            p_statementlist(ti, stmt, NULL);
                            addchild(dc, stmt);
                            t = tok(ti);
                            if(t->type == TOKTYPE_CLOSEBRACE) {
                                nexttok(ti);
                            }
                            else {
                                ERR("Syntax Error: Expected } but got");
                                synerr_rec(ti);
                            }
                        }
                        else {
                            ERR("Syntax Error: Expected { but got");
                            synerr_rec(ti);
                        }
                    }
                    else {
                        ERR("Syntax Error: Expected ) but got");
                        synerr_rec(ti);
                    }
                }
                else {
                    ERR("Syntax Error: Expected ( but got");
                    synerr_rec(ti);
                }
            }
            else {
                ERR("Error: Identifier must be a constructor or destructor: _ or ~_");
                synerr_rec(ti);
            }
            p_declist(ti, root);
            break;
        default:
            //epsilon production
            break;
    }
}

node_s *p_array(tokiter_s *ti, node_s *root)
{
    node_s *exp;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_OPENBRACKET) {
        nexttok(ti);
        if(!root) {
            root = MAKENODE();
            root->type = TOKTYPE_OPENBRACKET;
            root->att = TOKATT_DEFAULT;
            root->tok = t;
        }

        exp = p_optexpression(ti);
        addchild(root, exp);
        
        t = tok(ti);
        if(t->type == TOKTYPE_CLOSEBRACKET) {
            nexttok(ti);
            p_array(ti, root);
        }
        else {
            //syntax error
            ERR("Syntax Error: Expected ]");
            synerr_rec(ti);
        }
    }
    return root;
}

void p_mapdec(tokiter_s *ti, node_s **dec)
{
    node_s *op, *type;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_DMAP) {
        op = MAKENODE();
        op->type = TOKTYPE_DMAP;
        op->att = TOKATT_DEFAULT;
        op->tok = t;
        
        nexttok(ti);
        type = p_type(ti);
        addchild(op, *dec);
        addchild(op, type);
        *dec = op;
    }
}

node_s *p_typelist(tokiter_s *ti)
{
    node_s *type;
    tok_s *t = tok(ti);
    node_s *root = MAKENODE();
    
    root->type = TOKTYPE_TYPELIST;
    root->att = TOKATT_DEFAULT;
    
    switch(t->type) {
        case TOKTYPE_OPENPAREN:
            type = p_type(ti);
            addchild(root, type);
            p_typelist_(ti, root);
            break;
        case TOKTYPE_IDENT:
            t = peeknexttok(ti);
            if(t->type == TOKTYPE_COLON) {
                nexttok(ti);
                nexttok(ti);
            }
            type = p_type(ti);
            addchild(root, type);
            p_typelist_(ti, root);
            break;
        case TOKTYPE_VARARG:
            nexttok(ti);
            type = MAKENODE();
            type->type = TOKTYPE_VARARG;
            type->att = TOKATT_DEFAULT;
            addchild(root, type);
            p_typelist_(ti, root);
            break;
        default:
            //epsilon production
            break;
    }
    
    return root;
}

void p_typelist_(tokiter_s *ti, node_s *root)
{
    tok_s *t = tok(ti);
    node_s *type;
    
    if(t->type == TOKTYPE_COMMA) {
        t = nexttok(ti);
        switch(t->type) {
            case TOKTYPE_OPENPAREN:
                type = p_type(ti);
                addchild(root, type);
                p_typelist_(ti, root);
                break;
            case TOKTYPE_IDENT:
                t = peeknexttok(ti);
                if(t->type == TOKTYPE_COLON) {
                    nexttok(ti);
                    nexttok(ti);
                }
                type = p_type(ti);
                addchild(root, type);
                p_typelist_(ti, root);
                break;
            case TOKTYPE_VARARG:
                nexttok(ti);
                type = MAKENODE();
                type->type = TOKTYPE_VARARG;
                type->att = TOKATT_DEFAULT;
                addchild(root, type);
                p_typelist_(ti, root);
                break;
            default:
                ERR("Syntax Error: Expected type but got");
                break;
        }
    }
}

node_s *p_param(tokiter_s *ti)
{
    node_s *ident, *opt;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT) {
        ident = MAKENODE();
        ident->type = TOKTYPE_IDENT;
        ident->att = TOKATT_DEFAULT;
        ident->tok = t;
        
        nexttok(ti);
                
        opt = p_opttype(ti);
        addchild(ident, opt);
        
        if(addident(ti->scope->table, t->lex, opt)) {
            // adderr(ti, "Redeclaration", t->lex, t->line, "unique name", NULL);
        }
        
        p_assign(ti, &ident);
        return ident;
    }
    else {
        //syntax error
        ERR("Syntax Error: Expected identifier");
        synerr_rec(ti);
        return NULL;
    }
}

node_s *p_optparamlist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_IDENT || t->type == TOKTYPE_VARARG) {
        return p_paramlist(ti);
    }
    else {
        //epsilon production
        return NULL;
    }
}

node_s *p_paramlist(tokiter_s *ti)
{
    tok_s *t = tok(ti);
    node_s *root = MAKENODE(), *dec;

    root->type = TOKTYPE_PARAMLIST;
    root->att = TOKATT_DEFAULT;
    
    if(t->type == TOKTYPE_IDENT) {
        dec = p_param(ti);
        addchild(root, dec);
        p_paramlist_(ti, root);
    }
    else if(t->type == TOKTYPE_VARARG){
        nexttok(ti);
        dec = MAKENODE();
        dec->type = TOKTYPE_VARARG;
        dec->att = TOKATT_DEFAULT;
        dec->tok = t;
        addchild(root, dec);
        p_paramlist_(ti, root);
    }
    else {
        //syntax error
        ERR("Syntax Error: Expected identifier or ...");
        synerr_rec(ti);
    }
    
    return root;
}

void p_paramlist_(tokiter_s *ti, node_s *root)
{
    node_s *dec;
    tok_s *t = tok(ti);
    
    if(t->type == TOKTYPE_COMMA) {
        t = nexttok(ti);
        
        if(t->type == TOKTYPE_IDENT) {
            dec = p_param(ti);
            addchild(root, dec);
            p_paramlist_(ti, root);
        }
        else if(t->type == TOKTYPE_VARARG) {
            dec = MAKENODE();
            dec->type = TOKTYPE_VARARG;
            dec->att = TOKATT_DEFAULT;
            dec->tok = t;
            nexttok(ti);
            addchild(root, dec);
            p_paramlist_(ti, root);
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
    if(root && c) {
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

bool addident(rec_s *table[], char *key, bool isconst)
{
    unsigned index = pjwhash(key);
    rec_s **ptr = &table[index], *i = *ptr, *n;
    
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
    n->val = NULL;
    n->isconst = isconst;
    *ptr = n;
    return false;
}

void bindtype(scope_s *child, char *key, type_s *type)
{
    rec_s *i;
    unsigned index = pjwhash(key);

    while(child) {
        for(i = child->types[index]; i; i = i->next) {
            if(!strcmp(i->key, key)) {
                i->type = type;
                return;
            }
        }
        child = child->parent;
    }
}

bool bindexpr(scope_s *child, char *key, node_s *val)
{
    rec_s *i;
    unsigned index = pjwhash(key);
    
    while(child) {
        for(i = child->types[index]; i; i = i->next) {
            if(!strcmp(i->key, key)) {
                i->val= val;
                return true;
            }
        }
        child = child->parent;
    }
    return false;
}

bool addtype(scope_s *child, char *key, node_s *type)
{
    rec_s **table = child->types, *i;
    unsigned index = pjwhash(key);
    
    while(child) {
        for(i = child->types[index]; i; i = i->next) {
            if(!strcmp(i->key, key))
                return false;
        }
        child = child->parent;
    }
    addident(table, key, false);
    return true;
}

node_s *tablelookup(rec_s *table[], char *key)
{
    rec_s *i;
    unsigned index = pjwhash(key);
    
    for(i = table[index]; i; i = i->next) {
        if(!strcmp(i->key, key)) {
            return i->type;
        }
    }
    return NULL;
}

node_s *identlookup(scope_s *child, char *key)
{
    node_s *n;
    
    while(child) {
        if((n = tablelookup(child->table, key))) {
            return n;
        }
        child = child->parent;
    }
    return NULL;
}

bool addtype(scope_s *child, char *key, primtype_s *t)
{
    
}

node_s *typelookup(scope_s *child, char *key)
{
    node_s *n;
    
    while(child) {
        if((n = tablelookup(child->types, key))) {
            return n;
        }
        child = child->parent;
    }
    return NULL;
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
    }
}

node_s *getparentfunc(node_s *start)
{
    while(start) {
        if(start->type == TOKTYPE_CLOSURE)
            return start;
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

node_s *typecmp(node_s *t1, node_s *t2)
{
    return NULL;
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
    //asm("hlt");
    
    e = alloc(sizeof(*e));
    e->next = NULL;
    e->msg = s;
    if(ti->err)
        ti->ecurr->next = e;
    else
        ti->err = e;
    ti->ecurr = e;
}

void print_node(node_s *node)
{
    unsigned i;
    
    for(i = 0; i < node->nchildren; i++) {
        print_node(node->children[i]);
    }
    if(node->tok) {
        printf(" %s ", node->tok->lex);
    }
}