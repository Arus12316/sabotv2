#include "parse.h"
#include "server.h"
#include "connection.h"
#include "user.h"
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
    this->win = win;
    this->active = false;
    this->moveToThread(&thread);

    //temporary!!!
    this->server->userList = win->getUserList();
    this->server->selfUserList = win->getSelfUserList();
    this->server->messageInput = win->getMessageInput();

    connect(&thread, SIGNAL(started()), this, SLOT(sessionInit()));
    connect(this, SIGNAL(newUser(User *)), win, SLOT(newUser(User *)));
    connect(this, SIGNAL(newSelf(User *)), win, SLOT(newSelf(User *)));
    connect(this, SIGNAL(postMessage(message_s *)), win, SLOT(postMessage(message_s *)));
    connect(this, SIGNAL(deleteUser(Connection *, char *)), win, SLOT(deleteUser(Connection *, char* )));
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

void Connection::createAccount(const char name[], const char pass[], const char email[], quint8 r, quint8 g, quint8 b)
{
    static QByteArray incl = "_.", excl = "=&?";
    static QUrl url("http://www.xgenstudios.com/stickarena/stick_arena.php");
    static QNetworkRequest request;
    static QNetworkAccessManager netAccess;
    static QUrlQuery query;

    QString color;
    color.append(QString::number(r));
    color.append(QString::number(g));
    color.append(QString::number(b));

    query.clear();
    query.addQueryItem("email_address", email);
    query.addQueryItem("usercol", color);
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
    createAccount(name, pass, email, qrand() % 256, qrand() % 256, qrand() % 256);
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

        if(buf[0] == 'A') {
            user = new User(this, buf);
            user->isSelf = true;
            emit newSelf(user);
            if(!server->master) {
                server->master = this;
                messageBox = win->getMessageBox();
                emit newUser(user);
            }
        }

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
    char c, t;
    qint64 n;
    char *bptr, *mptr;
    User *u;
    char id[4], *idSignal;
    message_s *msg;

    while(sock->getChar(&c)) {

        gameBuf += c;

        if(!c) {
            n = gameBuf.size();
            bptr = gameBuf.data();

            if(server->master == this) {
                qDebug() << gameBuf;
                switch(*bptr) {
                    case 'U':
                        u = new User(this, bptr);
                        u->isSelf = false;
                        emit newUser(u);
                        break;
                    case 'D':
                        idSignal = new char[4];
                        idSignal[0] = *++bptr;
                        idSignal[1] = *++bptr;
                        idSignal[2] = *++bptr;
                        idSignal[3] = '\0';
                        emit deleteUser(this, idSignal);
                        break;
                    case 'C':
                        break;
                    case 'M':
                        id[0] = *++bptr;
                        id[1] = *++bptr;
                        id[2] = *++bptr;
                        id[3] = '\0';

                        msg = new message_s;
                        msg->type = *++bptr;
                        mptr = msg->body;
                        while(*bptr)
                            *mptr++ = *++bptr;

                        *mptr = '\0';
                        msg->sender = server->lookupUser(id);
                        msg->view = this;
                        emit postMessage(msg);
                        break;
                    }
            }
            else {
                if(*bptr == 'M') {
                    id[0] = *++bptr;
                    id[1] = *++bptr;
                    id[2] = *++bptr;
                    id[3] = '\0';

                    t = *++bptr;
                    if(t == 'P') {

                    }
                }
            }
            tmpout:
            gameBuf.clear();
        }
    }
}


void Connection::keepAlive()
{
    //sock->write(ackX0, sizeof ackX0);
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

void Connection::sendPublicMessage(message_s *msg)
{
    sendMessage(msg->body);
}

Connection::~Connection()
{
    active = false;
    delete[] username;
    delete[] password;
    sock->close();
    delete sock;
}

