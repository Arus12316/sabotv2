#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "database.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <unordered_map>

template <class T> class PCharHash;

template<> class PCharHash<const char *>
{
public:
    quint16 operator()(const char *c) const
    {

        return *c;
    }
};

namespace Ui {
class MainWindow;

}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QMutex lock;
    QWaitCondition cond;
    Database db;
    class Server *currServer;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setConn(class Connection *conn);
    void newTab(const char *server);
    class QListWidget *getMessageView();
    class QListWidget *getUserList();
    class QLineEdit *getMessageInput();
    class QListWidget *getSelfUserList();
    class QListWidget *getMiscView();
    class QListWidget *getGameView();
    class QPushButton *getSendButton();
    class QPushButton *getPmButton();
    class QLabel *getCurrUserLabel();
    class QLineEdit *getInputRaw();
    class QLineEdit *getPmAutoReply();
    bool calculatorOn();
    bool calculatorPM();
    bool autoReconnectIsChecked();
    void postMessage();
    int currServerIndex();
    void newTab(class Server *server);

    static void qStrCpy(char *dst, QString &src);

signals:
    void postMiscMessage(class Server *server, QString *msg);
    void sendPublicMessage(QString *msg);
    void sendPrivateMessage(struct message_s *msg);
    void sendRawMessage(QString msg);

public slots:
    void loginButtonPressed();
    void newUser(class User *);
    void newSelf(class UserSelf *);
    void postMessage(struct message_s *);
    void userDisconnected(class User *);

    void selfUserListItemChanged(class QListWidgetItem *curr, class QListWidgetItem *prev);
    void preparePublicMessage();
    void preparePrivateMessage();
    void prepareRawMessage();
    void postGameList(class Connection *conn);
    void postGeneralMain(Server *server, QString msg);
    void postGeneralMisc(Server *server, QString msg);
    void createAccount();
    void raid();
    void loginRecover(class Connection *last);
    void openProxyScan();


private:
    Ui::MainWindow *ui;

    class Connection *conn;
    class CreateAccount *ca;
    class ProxyScan *prox;
    class Raid *raidDialog;
};

#endif // MAINWINDOW_H
