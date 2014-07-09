#include "connection.h"

#define MAX_UNAME_PASS 20


const quint16 Connection::PORT = 1138;

/* Initial Packet Sent when logging in */
const char Connection::initSend[] = {
    0x30, 0x38, 0x48, 0x78, 0x4f, 0x39, 0x54, 0x64,
    0x43, 0x43, 0x36, 0x32, 0x4e, 0x77, 0x6c, 0x6e,
    0x31, 0x50, 0x00
};

/* General ACK 0 */
const char Connection::ackX0[] = {
    0x30, 0x00
};

/* General ACK 1 */
const char Connection::ackX1[] = {
    0x30, 0x63, 0x00
};

/* General ACK 2 */
const char Connection::ackX2[] = {
    0x30, 0x31, 0x00
};

/* This is sent to complete a login */
const char Connection::finishLogin[] = {
    0x30, 0x33, 0x5f, 0x00
};

const char Connection::charset1[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.";

const char Connection::charset2[] =
        "abcdefghijklmnopqrstuvwxyz1234567890";



const char *Connection::saServers[][2] = {
    {"2D Central", S_2D_CENTRAL},
    {"Paper Thin City", S_PAPER_THIN},
    {"Fine Line Island", S_FINE_LINE},
    {"U of SA", S_U_OF_SA},
    {"Flat World", S_FLAT_WORLD},
    {"Planar Outpost", S_PLANAR_OUTPOST},
    {"Mobius Metropolis", S_MOBIUS_METROPOLIS},
    {"EU Amsterdam", S_AMSTERDAM},
    {"Compatibility", S_COMPATABILITY},
    {"SS Lineage", S_SS_LINEAGE}
};

Connection::Connection(const char *host, QObject *parent) :
    QObject(parent)
{
    this->host = host;

    srand(time(NULL));
}

void Connection::randName(char *buf, unsigned short len)
{
    for(char *max = buf+len; buf < max; buf++)
        *buf = charset1[rand() % (sizeof charset1 - 1)];
    *buf = '\0';
}

void Connection::randEmail(char *buf, unsigned short len)
{
    char *max;
    unsigned short partition;
    const char *suffix;
    static const char *suffixList[] = {"net", "com", "gov", "edu", "org"};

    suffix = suffixList[rand() % (sizeof suffixList / sizeof *suffixList)];

    len -= 5;

    partition = 1 + (rand() % (len -1));

    for(max = buf + partition; buf < max; buf++)
        *buf = charset1[rand() % (sizeof charset1 - 2)];
    *buf++ = '@';

    for(max = buf + (len - partition); buf < max; buf++)
        *buf = charset1[rand() % (sizeof charset1 - 2)];
    *buf++ = '.';
    strcpy(buf, suffix);
}

void Connection::connect()
{
    sock.connectToHost(host, PORT, QTcpSocket::ReadWrite);
    if(sock.waitForConnected()) {

    }
}

void Connection::createAccount(char *name, char *pass, char *email, int color)
{

}

void Connection::createAccount(char *name, char *pass, char *email)
{
    createAccount(name, pass, email, rand());
}

void Connection::createAccount(char *name, char *pass)
{
    char email[MAX_UNAME_PASS + 1];


}

void Connection::createAccount(char *pass)
{

}

