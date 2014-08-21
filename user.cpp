#include "user.h"

#define BRIGHTNESS 100
#define MINPLAYERCOLORVALUE 6582527
#define MAXPLAYERCOLORVALUE 16777158

User::User(QObject *parent) :
    QObject(parent)
{
    color.setRgb(0, 0, 0, 255);

    id[0] = '0';
    id[1] = '0';
    id[2] = '0';

    modLevel = '0';
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

User::User(Connection *conn, const char *name, bool isdummy, QObject *parent)
{

    id[0] = '1';
    id[1] = '0';
    id[2] = '0';

    modLevel = '0';

    strcpy(this->name, name);
    this->conn = conn;
    color.setRgb(10, 10, 10, 20);
}


void User::parseData(const char *data)
{
    int i, n;
    char *ptr, type, col[4];
    int r, g, b, rgb;

    col[3] = '\0';
    type = *data++;

    id[0] = *data++;
    id[1] = *data++;
    id[2] = *data++;
    id[3] = '\0';

    //qDebug() << "id: " << id;

    for(ptr = (char *)data; *data == '#'; data++);

    n = MAX_UNAME_PASS - (data - ptr);

    for(i = 0; i < n; i++) {
        name[i] = *data++;
    }
    name[i] = '\0';

    //qDebug() << "name: " << name;

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


    if(!modLevel) {
        rgb = r << 16 ^ g << 8 ^ b;
        if(rgb < MINPLAYERCOLORVALUE || rgb > MAXPLAYERCOLORVALUE) {
            r = 222;
            g = 2;
            b = 2;
        }
    }
    r += BRIGHTNESS;
    if(r > 255)
        r = 255;

    g += BRIGHTNESS;
    if(g > 255)
        g = 255;

    b += BRIGHTNESS;
    if(b > 255)
        b = 255;

    color.setRgb(r, g, b, 255);

   // qDebug() << "Mod Level: " << modLevel;
}
