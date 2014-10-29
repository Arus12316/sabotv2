#ifndef PARSE_H_
#define PARSE_H_

#define SYM_TABLE_SIZE 19

#include "types.h"

typedef struct errlist_s errlist_s;
typedef struct tok_s tok_s;
typedef struct node_s node_s;
typedef struct rec_s rec_s;
typedef struct scope_s scope_s;

typedef type_s *ttable[SYM_TABLE_SIZE];

struct errlist_s
{
    errlist_s *next;
    char *msg;
};

typedef enum {
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
    TOKTYPE_SHIFT,
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
    TOKTYPE_VAR,
    TOKTYPE_LET,
    TOKTYPE_RETURN,
    TOKTYPE_BREAK,
    TOKTYPE_CONTINUE,
    TOKTYPE_CLASS,
    TOKTYPE_CHAR,
    TOKTYPE_UNNEG,
    TOKTYPE_STRUCTLITERAL,
    TOKTYPE_MAPLITERAL,
    TOKTYPE_STRUCTTYPE,
    TOKTYPE_COMPLEMENT,
    TOKTYPE_RANGE,
    TOKTYPE_STMTLIST,
    TOKTYPE_EXPRESSION,
    TOKTYPE_ROOT,
    TOKTYPE_STMTVOID,
    TOKTYPE_NEGATE,
    TOKTYPE_SUBSCRIPT,
    TOKTYPE_CALL,
    TOKTYPE_ARRAYLIT,
    TOKTYPE_CLOSURE,
    TOKTYPE_STATIC,
    TOKTYPE_CONSTRUCTOR,
    TOKTYPE_DESTRUCTOR,
    TOKTYPE_TYPELIST,
    TOKTYPE_PARAMLIST,
    TOKTYPE_OVERLOADOP
} toktype_e;

typedef enum {
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
    TOKATT_LSHIFT,
    TOKATT_RSHIFT,
    TOKATT_LCSHIFT,
    TOKATT_RCSHIFT,
} toktatt_e ;

struct tok_s
{
    toktype_e type;
    toktatt_e att;
    unsigned short line;
    char *lex;
};

struct node_s
{
    toktype_e type;
    toktatt_e att;
    short branch_complete;
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
    node_s *type;
    rec_s *next;
};

struct scope_s
{
    rec_s *table[SYM_TABLE_SIZE];
    ttable types;
    scope_s *parent;
    unsigned nchildren;
    scope_s **children;
};


extern errlist_s *parse(char *src);

extern void printerrs(errlist_s *err);

#endif
