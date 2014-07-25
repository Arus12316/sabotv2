#include "parse.h"

Parse::Parse(QObject *parent) :
    QObject(parent)
{
    tokens = NULL;
}

void Parse::tok(Parser::token_s *t)
{
    char *bptr = t->lexeme;

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
}



