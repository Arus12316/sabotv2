#include "user.h"
#include "parse.h"
#include "server.h"
#include "connection.h"
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QByteArray>
#include <QNetworkAccessManager>

#define SOCK_BUFSIZE 256
#define LOGIN_FLAG "09"

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

Connection::Connection(int server, MainWindow *win, QObject *parent) :
    QObject(parent)
{

    this->server = Server::servers[server];

    this->active = false;
    this->moveToThread(&thread);

    connect(&thread, SIGNAL(started()), this, SLOT(sessionInit()));
}


void Connection::randName(char *buf, ushort len)
{
    for(char *max = buf+len; buf < max; buf++)
        *buf = charset1[qrand() % (sizeof charset1 - 1)];
    *buf = '\0';
}

void Connection::randEmail(char *buf, ushort len)
{
    char *max;
    ushort partition;
    const char *suffix;
    static const char *suffixList[] = {"net", "com", "gov", "edu", "org"};

    suffix = suffixList[qrand() % (sizeof suffixList / sizeof *suffixList)];

    len -= 5;

    partition = 1 + (qrand() % (len - 1));

    for(max = buf + partition; buf < max; buf++)
        *buf = charset1[qrand() % (sizeof charset1 - 2)];
    *buf++ = '@';

    for(max = buf + (len - partition); buf < max; buf++)
        *buf = charset1[qrand() % (sizeof charset1 - 2)];

    *buf = '.';
    strcpy(buf+1, suffix);
}

void Connection::connect_()
{
    sock->connectToHost(server->getIP(), PORT, QTcpSocket::ReadWrite);
    if(sock->waitForConnected()) {

    }
}

void Connection::login(const char name[], const char pass[])
{
    this->username = qstrdup(name);
    this->password = qstrdup(pass);
    thread.start();
}

void Connection::sendMessage(const char msg[])
{
    size_t mlen = strlen(msg);
    char buf[256];

    if(mlen >= 256)
        perror("message to long");
    else {
        strcpy(buf, "9");
        strcat(buf, msg);
        //sock->write(conn->sock, buf, strlen(buf)+1, 0);
        sock->write(buf, strlen(buf) + 1);
    }
}

void Connection::createAccount(const char name[], const char pass[], const char email[], int color)
{
    static QByteArray incl = "_.", excl = "=&?";
    static QUrl url("http://www.xgenstudios.com/stickarena/stick_arena.php");
    static QNetworkRequest request;
    static QNetworkAccessManager netAccess;
    static QUrlQuery query;

    query.clear();
    query.addQueryItem("email_address", email);
    query.addQueryItem("usercol", QString::number(color));
    query.addQueryItem("userpass", pass);
    query.addQueryItem("username", name);
    query.addQueryItem("action", "create");

    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "I am Kim Jong Un. All Your Base are Belong To Us.");
    netAccess.post(request, url.toPercentEncoding(query.toString(),  excl, incl));
}

void Connection::createAccount(const char name[], const char pass[], const char email[])
{
    createAccount(name, pass, email, qrand());
}

void Connection::createAccount(const char name[], const char pass[])
{
    char email[MAX_UNAME_PASS + 1];

    randEmail(email, MAX_UNAME_PASS);
    createAccount(name, pass, email);
}

void Connection::createAccount(const char pass[])
{
    char name[MAX_UNAME_PASS + 1];

    randName(name, MAX_UNAME_PASS);
    createAccount(name, pass);

    qDebug() << "Created: " << name << endl;
}


/* Slots */


void Connection::sessionInit()
{
    enum { SLEEP_TIME = 5000 };
    qint64 n;
    char buf[SOCK_BUFSIZE];
    sock = new QTcpSocket;

    connect(sock, SIGNAL(connected()), this, SLOT(userConnected()));
    connect(sock, SIGNAL(disconnected()), this, SLOT(userDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorConnection(QAbstractSocket::SocketError)));

    sock->connectToHost(server->getIP(), PORT, QTcpSocket::ReadWrite);
    qDebug() << "Attempting to connect through tor" << endl;

    if(sock->waitForConnected()) {
        sock->write(initSend, sizeof initSend);

        sock->waitForReadyRead();

        n = sock->read(buf, sizeof buf);

        //sprintf in c++ umad
        sprintf(buf, LOGIN_FLAG "%s;%s", username, password);
        sock->write(buf, strlen(buf) + 1);

        sock->waitForReadyRead();

        n = sock->read(buf, sizeof buf);

        qDebug() << buf;


        sock->write(finishLogin, sizeof finishLogin);

        connect(sock, SIGNAL(readyRead()), this, SLOT(gameEvent()));
        connect(&timer, SIGNAL(timeout()), this, SLOT(keepAlive()));
        timer.start(SLEEP_TIME);

        active = true;
    }
}

void Connection::gameEvent()
{
    int i;
    char c;
    qint64 n;
    char *bptr;
    User *u;

    if(sock->getChar(&c)) {
        do {
            gameBuf += c;
        }
        while(sock->getChar(&c));

        if(!c) {
            n = gameBuf.size();
            qDebug() << gameBuf;
            for(bptr = gameBuf.data(); *bptr; bptr++) {
                switch(*bptr) {
                case 'U':
                    u = new User(this);

                    /* get uid */
                    u->id[0] = *++bptr;
                    u->id[1] = *++bptr;
                    u->id[2] = *++bptr;
                    u->id[3] = '\0';

                    for(n = 0; *++bptr == '#'; n++);

                    n = MAX_UNAME_PASS - n;
                    for(i = 0; i < n; i++)
                        u->name[i] = bptr[i];
                    bptr += n;
                    u->name[i] = '\0';

                    printf("\nuser: %s: %c%c%c\n", u->name, u->id[0], u->id[1], u->id[2]);
                    fflush(stdout);
                    *bptr = '\0';
                    break;
                case 'D':
                    break;
                case 'C':
                    break;
                case 'M':
                    break;
                }
            }
            gameBuf.clear();
        }
    }
}


void Connection::keepAlive()
{
    sock->write(ackX0, sizeof ackX0);
    sock->write(ackX2, sizeof ackX2);
}

void Connection::userConnected()
{
    qDebug() << "Connected!" << endl;
    emit signalConnected();
}

void Connection::userDisconnected()
{
    qDebug() << "Disconnecteds!" <<  endl;
}


void Connection::errorConnection(QAbstractSocket::SocketError error)
{
    qDebug() << "Connection Error: " << sock->errorString();

    if(error == QAbstractSocket::ProxyConnectionRefusedError) {
        qDebug() << "Tor is most likely not running. Attempting to Connect without tor.";
        sock->setProxy(QNetworkProxy::NoProxy);
        sock->connectToHost(server->getIP(), PORT, QTcpSocket::ReadWrite);
        qDebug() << "Attempting to connect" << endl;
    }
}

void Connection::test()
{
    qDebug() << "Slot called.";
}

Connection::~Connection()
{
    active = false;
    delete[] username;
    delete[] password;
    sock->close();
    delete sock;
}

