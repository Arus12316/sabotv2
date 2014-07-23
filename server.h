#ifndef SERVER_H
#define SERVER_H

#define UID_TABLE_SIZE 53
#define N_GAMESERVERS 10

#include <QObject>
#include <QListWidget>

class Server : public QObject
{
    Q_OBJECT
public:

    int index;

    class Connection *master;
    class Connection *currConn;
    class QListWidget *userList;
    class QLineEdit *messageInput;
    class QListWidget *selfUserList;
    class QListWidget *messageView;
    class QListWidget *miscView;
    class QListWidget *gameView;
    class QPushButton *sendButton;
    class QPushButton *pmButton;
    class QLabel *currUserLabel;

    explicit Server(int index, QObject *parent = 0);

    const char *getIP();
    const char *getName();

    static const char *toIP(const char key[]);

    void insertUser(class User *u);
    void deleteUser(const char *uid);
    class User *lookupUser(const char *uid);

    /* Server List (maps server names to their IP) */
    static const char *saServers[][2];

    static const char S_REGISTER[];
    static const char S_2D_CENTRAL[];
    static const char S_PAPER_THIN[];
    static const char S_FINE_LINE[];
    static const char S_U_OF_SA[];
    static const char S_FLAT_WORLD[];
    static const char S_PLANAR_OUTPOST[];
    static const char S_MOBIUS_METROPOLIS[];
    static const char S_AMSTERDAM[];
    static const char S_COMPATABILITY[];
    static const char S_SS_LINEAGE[];

    static const char KEY_2D[];
    static const char KEY_PTC[];
    static const char KEY_FLI[];
    static const char KEY_USA[];
    static const char KEY_FW[];
    static const char KEY_POU[];
    static const char KEY_MMET[];
    static const char KEY_EU[];
    static const char KEY_COMP[];
    static const char KEY_SS[];

    static Server *servers[N_GAMESERVERS];

signals:

public slots:

private:
    quint16 hashUid(const char *uid);

    struct hash_s {
        char key[4];
        class User *user;
        hash_s *next;
    }
    *utable[UID_TABLE_SIZE];
};

#endif // SERVER_H
