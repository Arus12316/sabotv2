#include "user.h"
#include <QListWidgetItem>

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
    : QObject(parent)
{
    this->conn = conn;
}

User::User(const char *buf, QObject *parent)
    : QObject(parent)
{
    this->conn = NULL;
    parseData(buf);
}

User::User(Connection *conn, const char *buf, QObject *parent)
    : QObject(parent)
{
    this->conn = conn;
    parseData(buf);
}

User::User(Connection *conn, const char *name, bool isdummy, QObject *parent)
    : QObject(parent)
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
    char *ptr, type, col[4], buf[32];
    int r, g, b, rgb;

    col[3] = '\0';
    type = *data++;

    id[0] = *data++;
    id[1] = *data++;
    id[2] = *data++;
    id[3] = '\0';

    for(ptr = (char *)data; *data == '#'; data++);

    n = MAX_UNAME_PASS - (data - ptr);

    for(i = 0; i < n; i++) {
        name[i] = *data++;
    }
    name[i] = '\0';

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

    for(ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    kills = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    deaths = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    wins = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    losses = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    roundsStarted = atoi(buf);

    isBallistick = *++data;
    data++;

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
}

User::~User()
{
    delete listEntry;
}

UserSelf::UserSelf(Connection *conn, const char *data, QObject *parent)
    : User(conn, parent)
{
    int i, n;
    char *ptr, type, col[4], buf[32];
    int r, g, b, rgb;

    col[3] = '\0';
    type = *data++;

    id[0] = *data++;
    id[1] = *data++;
    id[2] = *data++;
    id[3] = '\0';

    for(ptr = (char *)data; *data == '#'; data++);

    n = MAX_UNAME_PASS - (data - ptr);

    for(i = 0; i < n; i++) {
        name[i] = *data++;
    }
    name[i] = '\0';

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
    col[2] = *data;
    b = atoi(col);

    data += 10;

    for(ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    kills = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    deaths = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    wins = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    losses = atoi(buf);

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    roundsStarted = atoi(buf);

    isBallistick = 0;

    hasLabpass = *++data;

    for(data += 2, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    daysToExpire = atoi(buf);

    ticketWaiting = *++data;

    for(data++, ptr = buf; *data != ';'; data++)
        *ptr++ = *data;
    *ptr = '\0';
    zqkPmT = atoi(buf);

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
}

UserSelf::UserSelf(Connection *conn, const char *name, bool isdummy, QObject *parent)
    : User(conn, name, isdummy, parent)
{
    hasLabpass = '0';
    daysToExpire = 0;
    ticketWaiting = '0';
    zqkPmT = 0;
    selfEntry = NULL;
}

UserSelf::~UserSelf()
{
    delete selfEntry;
}
