#include "parse.h"
#include "server.h"
#include "connection.h"
#include "user.h"
#include "calc.h"
#include <unistd.h>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QByteArray>
#include <QLineEdit>
#include <QNetworkAccessManager>

#define SOCK_BUFSIZE 256
#define LOGIN_FLAG "09"
#define BAN_MSG "091"
#define PLAYER_FOUND_MSG "Player is located in"
#define CONN_TIMEOUT 15
#define TIMECHECK_INTERVAL 1000

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
    lastRead = time(NULL);

    this->server = Server::servers[server];
    this->win = win;
    this->active = false;
    this->moveToThread(&thread);
    this->timer.moveToThread(&thread);

    //temporary!!!
    this->server->userList = win->getUserList();
    this->server->selfUserList = win->getSelfUserList();
    this->server->messageInput = win->getMessageInput();
    this->server->miscView = win->getMiscView();
    this->server->gameView = win->getGameView();
    this->server->sendButton = win->getSendButton();
    this->server->pmButton = win->getPmButton();
    this->server->currUserLabel = win->getCurrUserLabel();
    this->server->inputRaw = win->getInputRaw();

    connect(&thread, SIGNAL(started()), this, SLOT(sessionInit()));
    connect(this, SIGNAL(newUser(User *)), win, SLOT(newUser(User *)));
    connect(this, SIGNAL(newSelf(UserSelf *)), win, SLOT(newSelf(UserSelf *)));
    connect(this, SIGNAL(postMessage(message_s *)), win, SLOT(postMessage(message_s *)));
    connect(this, SIGNAL(userDisconnected(User *)), win, SLOT(userDisconnected(User *)));
    connect(this, SIGNAL(postGameList(Connection *)), win, SLOT(postGameList(Connection *)));
    connect(this, SIGNAL(postGeneralMain(Server *, QString)), win, SLOT(postGeneralMain(Server *, QString)));
    connect(this, SIGNAL(postGeneralMisc(Server *, QString)), win, SLOT(postGeneralMisc(Server *, QString)));
    connect(this, SIGNAL(loginRecover(Connection *)), win, SLOT(loginRecover(Connection *)));
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

    printf("this is %p, sock is %p\n", this, sock);
    fflush(stdout);

    if(mlen >= 256)
        perror("message to long");
    else {
        strcpy(buf, "9");
        strcat(buf, msg);
        //sock->write(conn->sock, buf, strlen(buf)+1, 0);
        sock->write(buf, strlen(buf) + 1);
    }
}

void Connection::findUser(const char *name)
{
    char buf[32];

    buf[0] = '0';
    buf[1] = 'h';
    strcpy(&buf[2], name);

    sock->write(buf, strlen(buf) + 1);
}

void Connection::createAccount(const char name[], const char pass[], const char email[], quint8 r, quint8 g, quint8 b)
{
    static QByteArray incl = "_.", excl = "=&?";
    static QUrl url("http://www.xgenstudios.com/stickarena/stick_arena.php");
    static QNetworkRequest request;
    static QNetworkAccessManager netAccess;
    static QUrlQuery query;

    char color[10];

    sprintf(color, "%003u%003u%003u", r, g, b);

    query.clear();
    query.addQueryItem("email_address", email);
    query.addQueryItem("usercol", color);
    query.addQueryItem("userpass", pass);
    query.addQueryItem("use rname", name);
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
    enum { SLEEP_TIME = 10000 };
    qint64 n;
    char buf[SOCK_BUFSIZE];
    bool isfirst;
    sock = new QTcpSocket;
    UserSelf *userSelf;


    connect(sock, SIGNAL(connected()), this, SLOT(userConnected()));
    connect(sock, SIGNAL(disconnected()), this, SLOT(userDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorConnection(QAbstractSocket::SocketError)));

    sock->connectToHost(server->getIP(), PORT, QTcpSocket::ReadWrite);
    qDebug() << "Attempting to connect through tor" << endl;

    if(sock->waitForConnected()) {
        sock->write(initSend, sizeof initSend);

        sock->waitForReadyRead();

        n = sock->read(buf, sizeof buf);

        if(!strcmp(buf, "08")) {
          //  qDebug() << "IS FIRST";
           // isfirst = true;
        }

        //sprintf in c++ umad
        sprintf(buf, LOGIN_FLAG "%s;%s", username, password);
        sock->write(buf, strlen(buf) + 1);

        sock->waitForReadyRead();

        n = sock->read(buf, sizeof buf);

        if(!server->master) {
            server->master = this;
            server->currConn = this;
            server->messageView = win->getMessageView();
            connect(win, SIGNAL(sendPublicMessage(QString *)), this, SLOT(sendPublicMessage(QString *)));
            connect(win, SIGNAL(sendPrivateMessage(message_s *)), this, SLOT(sendPrivateMessage(message_s *)));
            connect(win, SIGNAL(sendRawMessage(QString)), this, SLOT(sendRaw(QString)));
        }

        if(!strcmp(buf, BAN_MSG)) {

            QString ban("[ ");

            ban += username;
            ban += " is banned ]";

            emit postGeneralMain(server, ban);

        }
        else if(buf[0] == 'A') {
            UserSelf *userSelf = new UserSelf(this, buf);
            user = userSelf;

            emit newUser(user);
            emit newSelf(userSelf);

            qDebug() << "hello 1!";

            sock->write(finishLogin, sizeof finishLogin);

            connect(sock, SIGNAL(readyRead()), this, SLOT(gameEvent()));
            connect(&timer, SIGNAL(timeout()), this, SLOT(keepAlive()));
            connect(&connTimer, SIGNAL(timeout()), this, SLOT(checkConnection()));
            timer.start(SLEEP_TIME);
            connTimer.start(TIMECHECK_INTERVAL);

            active = true;
        }
        else {
            userSelf = new UserSelf(this, username, true, this);
            user = userSelf;

            emit newUser(user);
            emit newSelf(userSelf);

            qDebug() << "hello 2!";

            sock->write(ackX0, sizeof ackX0);
            sock->write(ackX2, sizeof ackX2);

            if(isfirst)
                sock->write("02Z900_", sizeof "02Z900_");
            else
                sock->write(finishLogin, sizeof finishLogin);

            connect(sock, SIGNAL(readyRead()), this, SLOT(gameEvent()));
            connect(&timer, SIGNAL(timeout()), this, SLOT(keepAlive()));
            connect(&connTimer, SIGNAL(timeout()), this, SLOT(checkConnection()));
            timer.start(SLEEP_TIME);
            connTimer.start(TIMECHECK_INTERVAL);

            active = true;
        }
    }
}

void Connection::gameEvent()
{
    int i;
    char c, t;
    qint64 n;
    char *bptr, *mptr;
    User *u, *sender;
    char id[4];
    char type;
    calcres_s cres;
    message_s *msg;
    enum {CALC_TIME_MS=1500};

    lastRead = time(NULL);
    sock->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    while(sock->getChar(&c)) {
        gameBuf += c;

        if(!c) {
            n = gameBuf.size();
            bptr = gameBuf.data();

            if(server->master == this) {
                switch(*bptr) {
                    case '0':
                        switch(*++bptr) {
                            case '1':
                                while(*bptr) {
                                    for(mptr = bptr; *bptr != ';'; bptr++);
                                    *(bptr - 1) = '\0';
                                    gameList += mptr;
                                    bptr++;
                                }
                                emit postGameList(this);
                                listLock.lock();
                                listCond.wait(&listLock);
                                listLock.unlock();
                                break;
                            case '4':
                                break;
                            case '6':
                                break;
                            case '7':
                                break;
                            case '8':
                                break;
                            case '9':
                                break;
                            case 'a':
                                break;
                            case 'c':
                                break;
                            case 'e':
                            case 'f':
                                general.clear();
                                general += "[ BANNED: time: ";
                                mptr = &bptr[1];
                                //get time:
                                while(*++bptr != ';');
                                *bptr = '\0';
                                general += mptr;
                                //add message
                                general += "] ";
                                general += bptr + 1;
                                emit postGeneralMain(server, general);
                                break;
                            case 'g':
                                general.clear();
                                general += "[ warning: ";
                                general += &bptr[1];
                                general += " ]";
                                emit postGeneralMain(server, general);
                                break;
                            case 'h':
                                if(!strncmp(&bptr[1], PLAYER_FOUND_MSG, sizeof(PLAYER_FOUND_MSG) - 1)) {
                                    general.clear();
                                    general += "<";
                                    general += findBuf;
                                    general += "> has joined";
                                    general += &bptr[sizeof(PLAYER_FOUND_MSG)];
                                    emit postGeneralMisc(server, general);
                                }
                                break;
                            case 'j':
                                break;
                        }
                        break;
                    case 'U':
                        u = new User(this, bptr);
                        emit newUser(u);

                        //win->db.logUser(u);

                        //force synchronization
                        win->lock.lock();
                        win->cond.wait(&win->lock);
                        win->lock.unlock();
                        break;
                    case 'D':
                        id[0] = *++bptr;
                        id[1] = *++bptr;
                        id[2] = *++bptr;
                        id[3] = '\0';
                        u = server->lookupUser(id);

                        strcpy(findBuf, u->name);

                        emit userDisconnected(u);

                        //findUser(findBuf);

                        //force synchronization
                        win->lock.lock();
                        win->cond.wait(&win->lock);
                        win->lock.unlock();
                        break;
                    case 'C':
                        break;
                    case 'M':
                        id[0] = *++bptr;
                        id[1] = *++bptr;
                        id[2] = *++bptr;
                        id[3] = '\0';
                        type = *++bptr;

                        if(type == '9') {
                            switch(checkFlood(pubcap)) {
                                case FLOOD_START:
                                     emit postGeneralMain(server, "<<Entered Flood Mode (message spam detected)>>");
                                case FLOOD_STILL:
                                    return;
                                case FLOOD_END:
                                     emit postGeneralMain(server, "<<Leaving Flood Mode>>");
                                default:
                                    break;
                            }
                        }
                        else if(type == 'P') {
                            switch(checkFlood(pubcap)) {
                                case FLOOD_START:
                                     emit postGeneralMain(server, "<<Entered Flood Mode for PM (message spam detected)>>");
                                case FLOOD_STILL:
                                    return;
                                case FLOOD_END:
                                     emit postGeneralMain(server, "<<Leaving Flood Mode for PM>>");
                                default:
                                    break;
                            }
                        }
                        else {
                            switch(checkFlood(pubcap)) {
                                case FLOOD_START:
                                     emit postGeneralMisc(server, "<<Entered Flood Mode MISC (message spam detected)>>");
                                case FLOOD_STILL:
                                    return;
                                case FLOOD_END:
                                     emit postGeneralMisc(server, "<<Leaving Flood Mode MISC>>");
                                default:
                                    break;
                            }
                        }

                        msg = new message_s;
                        msg->type = type;
                        mptr = msg->body;
                        while(*bptr) {
                            *mptr++ = *++bptr;
                            if(mptr - msg->body == MAX_MSGLEN) {
                                mptr--;
                                break;
                            }
                        }
                        *mptr = '\0';
                        sender = server->lookupUser(id);
                        msg->sender = sender;
                        msg->view = this;
                        cres.status = -1;
                        if(msg->body[0] == ',' && win->calculatorOn()) {
                            cres = eval(&msg->body[1]);
                        }
                        emit postMessage(msg);
                        if(!cres.status) {
                            calcrep_s *rep = new calcrep_s;
                            rep->str = "result: ";
                            rep->str += cres.val;
                            rep->user = (type == 'P' || win->calculatorPM()) ? sender : NULL;
                            resQueue.enqueue(rep);
                            if(msg->sender == this->user)
                                QTimer::singleShot(CALC_TIME_MS, this, SLOT(sendRes()));
                            else
                                QTimer::singleShot(0, this, SLOT(sendRes()));
                            free(cres.val);
                        }
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
            qDebug() << gameBuf;
            gameBuf.clear();
        }
    }
}

void Connection::keepAlive()
{

    sock->write(ackX0, sizeof ackX0);
    sock->write(ackX2, sizeof ackX2);
    //sock->is
}

void Connection::userConnected()
{
    qDebug() << "Connected!" << endl;
    emit signalConnected();
}

void Connection::userDisconnected()
{
    QString msg("[ ");
    msg += username;
    msg += " disconnected ]";
    qDebug() << msg <<  endl;
    emit postGeneralMain(this->server, msg);
    emit loginRecover(this);
    thread.exit(0);
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

void Connection::sendPublicMessage(QString *msg)
{
    std::string sString = msg->toStdString();
    const char *cstr = sString.c_str();

    sendMessage(cstr);

    delete msg;
}

void Connection::sendPrivateMessage(message_s *msg)
{
    char msgbuf[256];

    msgbuf[0] = '0';
    msgbuf[1] = '0';

    strcpy(msgbuf+2, msg->receiver->id);
    msgbuf[5] = 'P';

    strcpy(msgbuf+6, msg->body);
    sock->write(msgbuf, strlen(msgbuf) + 1);

    delete msg;
}

void Connection::sendRaw(QString str)
{
    std::string stdstr = str.toStdString();
    const char *cstr = stdstr.c_str();

    sock->write(cstr, str.size() + 1);
}

void Connection::sendRes()
{
    if(resQueue.size()) {
        calcrep_s *rep = resQueue.dequeue();
        if(rep->user) {
            message_s *msg = new message_s;
            msg->receiver = rep->user;
            MainWindow::qStrCpy(msg->body, rep->str);
            sendPrivateMessage(msg);
        }
        else {
            sendPublicMessage(new QString(rep->str));
        }
        delete rep;
    }
}

void Connection::checkConnection()
{
    time_t dt = time(NULL) - lastRead;

    if(dt > CONN_TIMEOUT) {
        emit sock->disconnected();
    }
}

floodstate_e Connection::checkFlood(cap_s &cap)
{
    time_t nowTime = time(NULL);

    if(nowTime - cap.t <= cap.MIN_TIME_MS) {
        if(cap.count <= cap.SPAM_THRESHOLD)
            cap.count++;
    }
    else {
        if(cap.count >= cap.SPAM_THRESHOLD) {
            cap.t = nowTime;
            cap.count = cap.SPAM_THRESHOLD - 1;
            return FLOOD_END;
        }
        if(cap.count > cap.SPAM_THRESHOLD - 3)
            cap.count--;
        else if(cap.count) {
            cap.count = 0;
        }
    }
    cap.t = nowTime;
    if(cap.count >= cap.SPAM_THRESHOLD) {
        if(cap.count == cap.SPAM_THRESHOLD) {
            return FLOOD_START;
        }
        return FLOOD_STILL;
    }
    return FLOOD_NONE;
}


Connection::~Connection()
{
    active = false;
    if(sock->isOpen())
        sock->close();
    delete sock;
}
