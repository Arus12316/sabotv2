#ifndef USER_H
#define USER_H

#include "connection.h"
#include <QObject>
#include <QColor>
#include <QBrush>

#define MAX_UNAME_PASS 20

struct SelfData {
    char hasLabpass;
    int daysToExpire;
    char ticketWaiting;
    int zqkPmT;
};

class User : public QObject
{
    Q_OBJECT
public:
    char id[4];
    char name[MAX_UNAME_PASS + 1];
    QColor color;

    int kills;
    int deaths;
    int wins;
    int losses;
    int roundsStarted;
    char isBallistick;
    char modLevel;

    Connection *conn;
    SelfData *self;
    class QListWidgetItem *listEntry;
    class QListWidgetItem *selfEntry;

    User(Connection *conn, QObject *parent = 0);
    User(const char *buf, QObject *parent = 0);
    User(Connection *conn, const char *buf, QObject *parent = 0);
    User(Connection *conn, const char *name, bool isdummy, QObject *parent = 0);

    explicit User(QObject *parent = 0);

    ~User();

signals:

public slots:

private:
    void parseData(const char *data);
};

#endif // USER_H
