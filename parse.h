#ifndef PARSE_H
#define PARSE_H

#include <QObject>

/*
    statement :=  <expressionlist>

    <expressionlist> := <expression> <optnext>

    <optnext> := -> <expression> <optnext> | E

    <expression> := <object> | <object> := <expression> | <object>(<paramlist>)


*/



class Parse : public QObject
{
    Q_OBJECT
public:
    explicit Parse(QObject *parent = 0);


signals:

public slots:

};

#endif // PARSE_H
