#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlQuery>
#include <QSqlDatabase>


class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = 0);

    void logUser(class User *u);


signals:

public slots:

private:

    static QString nextQuery(const char **pptr);

    QSqlDatabase db;
    QSqlQuery checkUser;
    QSqlQuery insertUser;

};

#endif // DATABASE_H
