#include "user.h"
#include "parse.h"
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

Connection::Connection(Connection *master, const char *host, MainWindow *win, QObject *parent) :
    QObject(parent)
{
    for(int i = 0; i < N_GAMESERVERS; i++) {
        if(!strcmp(saServers[i][0], host)) {
            host = saServers[i][1];
            break;
        }
    }

    this->active = false;
    this->host = host;
    this->moveToThread(&thread);

    this->master = master;
    if(!master)
        this->utable = new hash_s[1][UID_TABLE_SIZE];
    else
        this->utable = master->utable;

    connect(this, SIGNAL(signalConnected()), win, SLOT(slotConnectedMsg()));
    connect(&thread, SIGNAL(started()), this, SLOT(userSession()));


    //connect(&sock, SIGNAL(error(QTcpSocket::SocketError))), this, SLOT(errorConnection(QTcpSocket::SocketError)));
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
    sock->connectToHost(host, PORT, QTcpSocket::ReadWrite);
    if(sock->waitForConnected()) {

    }
}

void Connection::atomicWrite(const char *data, qint64 n)
{
    mutex.lock();
    sock->write(data, n);
    mutex.unlock();
}

void Connection::atomicFlush()
{
    mutex.lock();
    sock->flush();
    mutex.unlock();
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
void Connection::userSession()
{
    User *u;
    bool notExpire;
    qint64 n;
    int i;
    QByteArray array;
    char buf[SOCK_BUFSIZE], *bptr, c;

    sock = new QTcpSocket;

    connect(sock, SIGNAL(connected()), this, SLOT(userConnected()));
    connect(sock, SIGNAL(disconnected()), this, SLOT(userDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorConnection(QAbstractSocket::SocketError)));

    sock->connectToHost(host, PORT, QTcpSocket::ReadWrite);
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

        sock->write(finishLogin, sizeof finishLogin);

        keepAlive = new KeepAlive(this);

        active = true;

        while(active) {
            do  {
                while(!sock->getChar(&c)) {
                    if(!sock->waitForReadyRead()) {
                        return; /* temporary way to make stderr shutup when program closes */
                    }
                }
                array += c;
            } while(c);

            n = array.size();

            for(bptr = array.data(); *bptr; bptr++) {
                putchar(*bptr);
                switch(*bptr) {
                    case 'U':
                        u = new User(this);

                        /* get uid */
                        u->id[0] = *++bptr;
                        u->id[1] = *++bptr;
                        u->id[2] = *++bptr;
                        
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
            puts("%\n\n");
            fflush(stdout);
            array.clear();
        }
    }
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
        sock->connectToHost(host, PORT, QTcpSocket::ReadWrite);
        qDebug() << "Attempting to connect" << endl;
    }
}

quint16 Connection::hashUid(const char uid[3])
{
    quint32 h = 0, g;

    h = (h << 4) + uid[0];
    if((g = h & 0xf0000000)) {
        h ^= (g >> 24);
        h ^= g;
    }
    h = (h << 4) + uid[1];
    if((g = h & 0xf0000000)) {
        h ^= (g >> 24);
        h ^= g;
    }
    h = (h << 4) + uid[2];
    if((g = h & 0xf0000000)) {
        h ^= (g >> 24);
        h ^= g;
    }
    return h % UID_TABLE_SIZE;
}


Connection::~Connection()
{
    active = false;
    delete[] username;
    delete[] password;
    delete keepAlive;
    sock->close();
    delete sock;

    if(!master) {

    }
}

KeepAlive::KeepAlive(Connection *conn)
{
    this->conn = conn;
    this->active = true;
    this->moveToThread(&thread);

    connect(&thread, SIGNAL(started()), this, SLOT(keepAlive()));
    thread.start();
}

void KeepAlive::keepAlive()
{
    enum { SLEEP_TIME = 5000 };

    while(active) {
        conn->atomicWrite(Connection::ackX0, sizeof Connection::ackX0);
        conn->atomicWrite(Connection::ackX2, sizeof Connection::ackX2);
        conn->atomicFlush();
        QThread::msleep(SLEEP_TIME);
    }
}

KeepAlive::~KeepAlive()
{
    active = false;
    thread.deleteLater();
}
