#ifndef CONNECTION_H
#define CONNECTION_H

#include "mainwindow.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QTcpSocket>
#include <QNetworkProxy>

class Connection;

class KeepAlive : public QObject
{
    Q_OBJECT

public:
    KeepAlive(Connection *conn);
    ~KeepAlive();

public slots:
    void keepAlive();


private:
    bool active;
    Connection *conn;
    QThread thread;
};

class Connection : public QObject
{
    Q_OBJECT

public:
    /* General ACK 0 */
    static const char ackX0[];

    /* General ACK 1 */
    static const char ackX1[];

    /* General ACK 2 */
    static const char ackX2[];

    explicit Connection(int server, MainWindow *win, QObject *parent = 0);

    ~Connection();

    /*
     * These pseudorandom generators need to be seeded.
     */
    static void randName(char *buf, ushort len);
    static void randEmail(char *buf, ushort len);

    void connect_();

    void atomicWrite(const char *data, qint64 n);
    void atomicFlush();

    void login(const char name[], const char pass[]);

    void sendMessage(const char msg[]);

    /*
     * Account Creating methods. Methods with less parameters generate the values randomly. Use
     * only in 1 thread. These are not reentrant.
     */
    static void createAccount(const char name[], const char pass[], const char email[], int color);
    static void createAccount(const char name[], const char pass[], const char email[]);
    static void createAccount(const char name[], const char pass[]);
    static void createAccount(const char pass[]);

signals:
    void signalConnected();

public slots:

    void userSession();

    void userConnected();
    void userDisconnected();
    void errorConnection(QAbstractSocket::SocketError error);


private:

    quint16 hashUid(const char *uid);
    void insertUser(class User *u);

    QTcpSocket *sock;
    QMutex mutex;
    KeepAlive *keepAlive;
    char uid[4];
    QThread thread;
    char *username;
    char *password;
    bool active;
    class Server *server;

    static const quint16 PORT;

    /* Initial Packet Sent when logging in */
    static const char initSend[];

    /* This is sent to complete a login */
    static const char finishLogin[];

    /* Char maps for generating random strings */
    static const char charset1[];
    static const char charset2[];
};

#endif // CONNECTION_H
