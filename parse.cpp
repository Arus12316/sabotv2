#include "parse.h"

Parse::Parse(QObject *parent) :
    QObject(parent)
{
    tokens = NULL;
}

Parser::token_s *Parse::tok()
{
    char *bptr;
    Parser::token_s *tok = new Parser::token_s;
    bptr = tok->lexeme;

    switch(*currptr) {
        case '(':
            *bptr = '(';
            *(bptr + 1) = '\0';
            break;
        case ')':
            break;
        case '.':
            break;
        case '-':
            if(*(currptr + 1) == '>') {

                currptr++;
            }
            else {

            }
            break;
        case '"':
            break;
        case '_':

            break;
        default:
            if(isdigit(*currptr)) {

            }
            else if(isalpha(*currptr)) {

            }
            break;
    }
    return tok;
}

void Parse::add_tok(Parser::token_s *t)
{
    if(!tokens) {
        tokens = t;
    }
}

