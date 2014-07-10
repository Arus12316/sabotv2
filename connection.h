#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QTcpSocket>

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

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(const char *host, QObject *parent = 0);

    /*
     * These pseudorandom generators need to be seeded.
     */
    static void randName(char *buf, ushort len);
    static void randEmail(char *buf, ushort len);

    void connect();

    void login(char *name, char *pass);

    static void createAccount(const char name[], const char pass[], const char email[], int color);
    static void createAccount(const char name[], const char pass[], const char email[]);
    static void createAccount(const char name[], const char pass[]);
    static void createAccount(const char pass[]);

signals:

public slots:

private:


    QTcpSocket sock;
    QString host;

    static const quint16 PORT;

    /* Initial Packet Sent when logging in */
    static const char initSend[];

    /* General ACK 0 */
    static const char ackX0[];

    /* General ACK 1 */
    static const char ackX1[];

    /* General ACK 2 */
    static const char ackX2[];

    /* This is sent to complete a login */
    static const char finishLogin[];

    /* Server List (maps server names to their IP) */
    static const char *saServers[][2];

    static const char charset1[];
    static const char charset2[];
};

#endif // CONNECTION_H
