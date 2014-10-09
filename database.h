#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QThread>


class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database();

signals:
    void logUser(class User *user);

private slots:
    void logUserSlot(class User *u);

private:

    static QString nextQuery(const char **pptr);

    QSqlDatabase db;
    QSqlQuery checkUser;
    QSqlQuery insertUser;
    QThread thread;

};

#endif // DATABASE_H
