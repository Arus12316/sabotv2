#include "database.h"
#include "schema.h"
#include "user.h"
#include <QSqlError>
#include <QSqlResult>
#include <QFile>
#include <QDebug>

Database::Database(QObject *parent) :
    QObject(parent)
{
    QFile f;
    QString query;
    QSqlQuery q;
    const char *ptr;

    db = QSqlDatabase::addDatabase("QSQLITE", "sabot.db");

    f.setFileName("sabot.db");
    if(!f.exists()) {
        db.open();
        ptr = schema;
        while(ptr) {
            query = nextQuery(&ptr);
            db.exec(query);
        }
    }
    else {
        if(!db.open()) {
            qDebug() << "Open Failed " << db.lastError().text();
        }
        else {
            qDebug() << "Open Succeeded";
        }
    }

    checkUser = QSqlQuery(db);
    insertUser = QSqlQuery(db);

    if(!checkUser.prepare("SELECT id FROM sabot.user WHERE name=?;")) {
        qDebug() << "Error: " << db.lastError().text();
    }
    if(!insertUser.prepare("INSERT INTO sabot.user(name) VALUES(?);")) {
        qDebug() << "Error: " << db.lastError().text();
    }
}

void Database::logUser(User *u)
{
    checkUser.addBindValue(u->name);
    if(!checkUser.exec()) {
        qDebug() << db.lastError().text();
    }

    if(checkUser.size() == 0) {
        qDebug() << "Got 0!";
        insertUser.addBindValue(u->name);
        insertUser.exec();
        insertUser.clear();
    }
    checkUser.clear();
}

QString Database::nextQuery(const char **pptr)
{
    QString query;
    const char *ptr = *pptr;

    do {
        query += *ptr++;
        if(!*ptr) {
            *pptr = NULL;
            return query;
        }
    }
    while(*ptr != ';');
    *pptr = ptr;
    return query;
}
