#include "database.h"
#include "schema.h"
#include "user.h"
#include <QSqlError>
#include <QSqlResult>
#include <QFile>
#include <QDebug>

Database::Database() :
    QObject(),
    db(QSqlDatabase::addDatabase("QSQLITE", "sabot.db")),
    checkUser(db),
    insertUser(db)
{
    QFile f;
    QString query;
    QSqlQuery q;
    const char *ptr;

    db.setDatabaseName("sabot.db");
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

    checkUser.clear();
    insertUser.clear();

    if(!checkUser.prepare("SELECT id FROM user WHERE name=?")) {
        qDebug() << "Error 1: " << checkUser.lastError().text();
    }
    if(!insertUser.prepare("INSERT INTO user(name) VALUES(?)")) {
        qDebug() << "Error 2: " << db.lastError().text();
    }
    this->moveToThread(&thread);
    connect(this, SIGNAL(logUser(User *)), this, SLOT(logUserSlot(User *)));
    thread.start();
}

void Database::logUserSlot(User *u)
{
    db.transaction();
    checkUser.addBindValue(u->name);
    qDebug() << "bound values: " << checkUser.boundValues().size() << " val " << u->name;
    if(!checkUser.exec()) {
        qDebug() << "exec failed: " << db.lastError().text();
    }
    qDebug() << "log user called: " << checkUser.size() << " isactive: " << checkUser.isActive() << " iselect: " << checkUser.isSelect() << " isvalid " << checkUser.isValid();
    if(checkUser.size() == 0) {
        qDebug() << "Got 0!";
        insertUser.addBindValue(u->name);
        insertUser.exec();
        insertUser.finish();
        insertUser.clear();
    }
    checkUser.finish();
    checkUser.clear();
    db.commit();
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
