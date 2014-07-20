#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QMutex>
#include <QWaitCondition>
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
    void postMessage();

    void newTab(class Server *server);

signals:
    void postMiscMessage(class Server *server, QString *msg);

public slots:
    void loginButtonPressed();
    void newUser(class User *);
    void newSelf(class User *);
    void postMessage(struct message_s *);
    void userDisconnected(class User *);

    void selfUserListItemChanged(class QListWidgetItem *curr, class QListWidgetItem *prev);
    void sendMessage();

private:
    Ui::MainWindow *ui;

    class Connection *conn;
};

#endif // MAINWINDOW_H
