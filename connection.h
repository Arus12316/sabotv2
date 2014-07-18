#ifndef CONNECTION_H
#define CONNECTION_H

#include "mainwindow.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QTcpSocket>
#include <QNetworkProxy>

struct message_s {
    class User *sender;
    class Connection *view;
    char body[148];
    char type;
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

    class Server *server;

    explicit Connection(int server, MainWindow *win, QObject *parent = 0);

    ~Connection();

    /*
     * These pseudorandom generators need to be seeded.
     */
    static void randName(char *buf, ushort len);
    static void randEmail(char *buf, ushort len);

    void connect_();

    void login(const char name[], const char pass[]);

    void sendMessage(const char msg[]);

    /*
     * Account Creating methods. Methods with less parameters generate the values randomly. Use
     * only in 1 thread. These are not reentrant.
     */
    static void createAccount(const char name[], const char pass[], const char email[], quint8 r, quint8 g, quint8 b);
    static void createAccount(const char name[], const char pass[], const char email[]);
    static void createAccount(const char name[], const char pass[]);
    static void createAccount(const char pass[]);

signals:
    void signalConnected();

    void newUser(class User *);
    void newSelf(class User *);

    void postMessage(message_s *msg);
    void deleteUser(Connection *conn, char *id);

public slots:

    void sessionInit();
    void gameEvent();
    void keepAlive();

    void userConnected();
    void userDisconnected();
    void errorConnection(QAbstractSocket::SocketError error);
    void test();
    void sendPublicMessage(message_s *msg);


private:

    quint16 hashUid(const char *uid);
    void insertUser(class User *u);

    QTcpSocket *sock;
    char uid[4];
    QThread thread;
    char *username;
    char *password;
    bool active;
    QByteArray gameBuf;
    QTimer timer;
    class User *user;
    MainWindow *win;

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
