#include "user.h"

User::User(QObject *parent) :
    QObject(parent)
{
}

User::User(Connection *conn, QObject *parent)
{
    this->conn = conn;
}
