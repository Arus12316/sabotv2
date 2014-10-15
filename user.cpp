#include "user.h"

#define BRIGHTNESS 100
#define MINPLAYERCOLORVALUE 6582527
#define MAXPLAYERCOLORVALUE 16777158

User::User(QObject *parent) :
    QObject(parent)
{
    this->self = NULL;
    this->selfEntry = NULL;
    color.setRgb(0, 0, 0, 255);

    id[0] = '0';
    id[1] = '0';
    id[2] = '0';

    modLevel = '0';
}

User::User(Connection *conn, QObject *parent)
{
    this->self = NULL;
    this->selfEntry = NULL;
    this->conn = conn;
}

User::User(const char *buf, QObject *parent)
{
    this->selfEntry = NULL;
    this->conn = NULL;
    parseData(buf);
}

User::User(Connection *conn, const char *buf, QObject *parent)
{
    this->selfEntry = NULL;
    this->conn = conn;
    parseData(buf);
}

User::User(Connection *conn, const char *name, bool isdummy, QObject *parent)
{
    this->self = NULL;
    this->selfEntry = NULL;

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

    if(type == 'A')
        data += 9;

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

    if(type == 'A') {
        isBallistick = 0;
        self = new SelfData;

        self->hasLabpass = *++data;

        for(data += 2, ptr = buf; *data != ';'; data++)
            *ptr++ = *data;
        *ptr = '\0';
        self->daysToExpire = atoi(buf);

        self->ticketWaiting = *++data;

        for(data++, ptr = buf; *data != ';'; data++)
            *ptr++ = *data;
        *ptr = '\0';
        self->zqkPmT = atoi(buf);
    }
    else {
        isBallistick = *++data;
        data++;
        self = NULL;
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
}

User::~User()
{
    if(self)
        delete self;
}
