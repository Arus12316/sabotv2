#ifndef USER_H
#define USER_H

#include "connection.h"
#include <QObject>

#define MAX_UNAME_PASS 20


class User : public QObject
{
    Q_OBJECT
public:
    char id[3];
    char name[MAX_UNAME_PASS + 1];


    User(Connection *conn, QObject *parent = 0);


    explicit User(QObject *parent = 0);

signals:

public slots:

private:
    Connection *conn;

};

#endif // USER_H
