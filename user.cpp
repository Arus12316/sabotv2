#include "user.h"

User::User(QObject *parent) :
    QObject(parent)
{
}

User::User(Connection *conn, QObject *parent)
{
    this->conn = conn;
}

User::User(const char *buf, QObject *parent)
{
    this->conn = NULL;
    parseData(buf);
}

User::User(Connection *conn, const char *buf, QObject *parent)
{
    this->conn = conn;
    parseData(buf);
}

void User::parseData(const char *data)
{
    int i, n;
    char *ptr, type, col[4];
    int r, g, b;

    col[3] = '\0';
    type = *data++;

    id[0] = *data++;
    id[1] = *data++;
    id[2] = *data++;
    id[3] = '\0';

    qDebug() << "id: " << id;

    for(ptr = (char *)data; *data == '#'; data++);

    n = MAX_UNAME_PASS - (data - ptr);

    for(i = 0; i < n; i++) {
        name[i] = *data++;
    }
    name[i] = '\0';

    qDebug() << "name: " << name;

    col[0] = *data++;
    col[1] = *data++;
    col[2] = *data++;
    r = atoi(col);

    col[0] = *data++;
    col[1] = *data++;
    col[2] = *data++;
    g = atoi(col);

    col[0] = *data++;
    col[1] = *data++;
    col[2] = *data++;
    b = atoi(col);

   // qDebug() << "RGB( "<< r << ", " << g << ", " << b << " )";

    color.setRgb(r, g, b);
    brush.setColor(color);
    brush.setStyle(Qt::SolidPattern);

    for(ptr = field1; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';

    //qDebug() << "field 1: " << field1;

    for(data++, ptr = field2; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';

    //qDebug() << "field 2: " << field2;


    for(data++, ptr = field3; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';

    //qDebug() << "field 3: " << field3;


    for(data++, ptr = field4; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';

    //qDebug() << "field 4: " << field4;


    for(data++, ptr = field5; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';

   // qDebug() << "field 5: " << field5;


    for(data++, ptr = field6; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';

   // qDebug() << "field 6: " << field6;

    if(type == 'A') {
        for(data++, ptr = field7; *data != ';'; data++)
            *ptr++ = *data;
        *ptr = '\0';

       // qDebug() << "field 7: " << field7;

        for(data++, ptr = field8; *data != ';'; data++)
            *ptr++ = *data;
        *ptr = '\0';

       // qDebug() << "field 8: " << field8;

        for(data++, ptr = field9; *data != ';'; data++)
            *ptr++ = *data;
        *ptr = '\0';

       // qDebug() << "field 9: " << field9;
    }

    modLevel = *++data;

   // qDebug() << "Mod Level: " << modLevel;
}
