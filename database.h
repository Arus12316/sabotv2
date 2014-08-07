#ifndef DATABASE_H
#define DATABASE_H

class Database
{
public:
    Database();

private:
    class QSqlDatabase *db;
};

#endif // DATABASE_H
