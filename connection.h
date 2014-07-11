#ifndef CONNECTION_H
#define CONNECTION_H

#include "mainwindow.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QTcpSocket>

#define N_GAMESERVERS 11

#define S_REGISTER          "67.19.145.10"

#define S_2D_CENTRAL        "74.86.43.9"
#define S_PAPER_THIN        "74.86.43.8"
#define S_FINE_LINE         "67.19.138.234" //(?)
#define S_U_OF_SA           "67.19.138.236"
#define S_FLAT_WORLD        "74.86.3.220"
#define S_PLANAR_OUTPOST    "67.19.138.235"
#define S_MOBIUS_METROPOLIS "74.86.3.221"
#define S_AMSTERDAM         "94.75.214.10"
#define S_COMPATABILITY     "74.86.3.222"
#define S_QUICKSTART        "67.19.138.236"
#define S_SS_LINEAGE        "74.86.43.10"

#define TEST_2D "2D Central"
#define TEST_PTC "Paper Thin City"
#define TEST_FLI "Fine Line Island"
#define TEST_USA "U of SA"
#define TEST_FW "Flat World"
#define TEST_POU "Planar Outpost"
#define TEST_MMET "Mobius Metropolis"
#define TEST_EU "Eu Amsterdam"
#define TEST_COMP "Compatibility"
#define TEST_SS "SS Lineage"

class KeepAlive : public QObject
{
    Q_OBJECT

public:
    KeepAlive(class Connection *conn);

public slots:
    void keepAlive();

private:
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


    explicit Connection(const char *host, MainWindow *win, QObject *parent = 0);

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
    QTcpSocket *sock;
    QMutex mutex;
    KeepAlive *keepAlive;
    QString host;
    char uid[3];
    QThread thread;
    char *username;
    char *password;

    static const quint16 PORT;

    /* Initial Packet Sent when logging in */
    static const char initSend[];

    /* This is sent to complete a login */
    static const char finishLogin[];

    /* Server List (maps server names to their IP) */
    static const char *saServers[][2];

    /* Char maps for generating random strings */
    static const char charset1[];
    static const char charset2[];
};

#endif // CONNECTION_H
