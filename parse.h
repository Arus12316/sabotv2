#ifndef PARSE_H
#define PARSE_H

#include <QObject>

/*
    <statementlist> := <statement> <statementlist'>
    <statementlist> := <statement> <statementlist> | E

    <statement> :=  <expressionlist> | <control> | <dec> | return <expression>

    <expressionlist> := <expression> <expressionlist'>

    <expressionlist'> := <expression> <expressionlist'> | E

    <expression> := <simple_expression> <expression'>

    <expression'> := relop <simple_expression>
                    |
                    E

    <simple_expression> := <sign> <term> <simple_expression'>
                           |
                           <term> <simple_expression'>

    <simple_expression'> := addop <term> <simple_expression>
                            |
                            E

    <term> := <factor> <term'>

    <term'> := mulop <factor> <term'>
                |
                E

    <factor> := id <factor'>
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
                <initializer>
                |
                @ (<paramlist>) { <statementlist> } <optcall>

    <factor'> := [ <expression> ] | . id <factor'> | ( <paramlist> ) | E

    <optcall> := ( <paramlist> ) | E

    <sign> + | -


    <paramlist> := <expression> <paramlist'>
    <paramlist'> := , <expression> <paramlist'> | E

    <control> := if <expression> then <statementlist> <elseif>
                 |
                 while <expression> do <statementlist> endwhile
                 |
                 for id <- id do <statementlist> endfor

    <elseif> := else <statementlist> endif | elif <expression> then <statementlist> endif

    <dec> := var id := <expression>

    <initializer> := { <paramlist> <optmap> }

    <optmap> := -> <expression>

*/

namespace Parser {

#define TOK_CHUNK_SIZE 16


//typedef struct token_s token_s;

enum tok_type_e {
    TOK_NUM,
    TOK_ID,
};

enum tok_attr_e {
    ATT_DEFAULT,
    ATT_INT,
    ATT_REAL,
};

struct token_s {
    char *lexeme;
    tok_type_e type;
    tok_type_e att;
};

struct tokchunk_s {
    int size;
    token_s toks[TOK_CHUNK_SIZE];
    tokchunk_s *next;
};

class Parse;

}

class Parse : public QObject
{
    Q_OBJECT
public:

    explicit Parse(char *src, QObject *parent = 0);

signals:

public slots:

private:
    void tok(Parser::token_s *t);

    void addTok(char *lexeme, size_t len, int type, int att);

    char *src;
    char *currptr;
    Parser::tokchunk_s *head;
    Parser::tokchunk_s *currchunk;
};

#endif // PARSE_H
