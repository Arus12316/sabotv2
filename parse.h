#ifndef PARSE_H
#define PARSE_H

#include <QObject>

/*
    statement :=  <expressionlist>

    <expressionlist> := <expression> <optnext>

    <optnext> := -> <expression> <optnext> | E

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

    <factor'> := [ <expression> ] | . id <factor'> | ( <expressionlist> ) | E

    <sign> + | -


*/

namespace Parser {

#define MAX_TOK_SIZE 32

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

    char lexeme[MAX_TOK_SIZE];
    tok_type_e type;
    tok_type_e att;
    token_s *next;
};

class Parse;

}

class Parse : public QObject
{
    Q_OBJECT
public:
    explicit Parse(QObject *parent = 0);

signals:

public slots:

private:
    void tok(Parser::token_s *t);

    Parser::token_s *tokens;
    char *currptr;
};

#endif // PARSE_H
