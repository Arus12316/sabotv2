#ifndef USER_H
#define USER_H

#include "connection.h"
#include <QObject>
#include <QColor>
#include <QBrush>

#define MAX_UNAME_PASS 20

class User : public QObject
{
    Q_OBJECT
public:
    char id[4];
    char name[MAX_UNAME_PASS + 1];
    QColor color;

    char field1[32];
    char field2[32];
    char field3[32];
    char field4[32];
    char field5[32];
    char field6[32];
    char field7[32];
    char field8[32];
    char field9[32];

    char modLevel;
    Connection *conn;
    bool isSelf;
    class QListWidgetItem *listEntry;
    class QListWidgetItem *selfEntry;

    User(Connection *conn, QObject *parent = 0);
    User(const char *buf, QObject *parent = 0);
    User(Connection *conn, const char *buf, QObject *parent = 0);
    User(Connection *conn, const char *name, bool isdummy, QObject *parent = 0);

    explicit User(QObject *parent = 0);

signals:

public slots:

private:

    void parseData(const char *data);
};

#endif // USER_H
