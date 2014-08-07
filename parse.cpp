#include "parse.h"

enum {
    TOKTYPE_IDENT,
    TOKTYPE_NUM,
    TOKTYPE_STRING,
    TOKTYPE_COMMA,
    TOKTYPE_LAMBDA,
    TOKTYPE_MAP,
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

Parse::Parse(char *src, QObject *parent) :
    QObject(parent)
{
    this->tokens = NULL;
    this->src = src;
}

void Parse::tok(void)
{
    char *bptr;
    head = currchunk = new Parser::tokchunk_s;
    currchunk->size = 0;
    currchunk->next = NULL;

    switch(*currptr) {
        case ',':
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
    }
}

void Parse::addTok(char *lexeme, size_t len, int type, int att)
{
    char *lex;
    int size = currchunk->size;

    if(size == TOK_CHUNK_SIZE) {
        currchunk->next = new Parser::tokchunk_s;
        currchunk = currchunk->next;
        currchunk->size = size = 0;
        currchunk->next = NULL;
    }

    lex = new char[len + 1];
    strcpy(lex, lexeme);
    currchunk->toks[size].lexeme = lex;
    currchunk->toks[size].type = type;
    currchunk->toks[size].att = att;
    currchunk->size++;
}



