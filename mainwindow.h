#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
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
    class Server *currServer;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setConn(class Connection *conn);
    void newTab(const char *server);
    class QListWidget *getMessageView();
    class QListWidget *getUserList();
    class QLineEdit *getMessageInput();
    class QListWidget *getSelfUserList();
    void postMessage();

    void newTab(class Server *server);

public slots:
    void loginButtonPressed();
    void newUser(class User *);
    void newSelf(class User *);
    void postMessage(struct message_s *);
    void deleteUser(class Connection *conn, char *id);

    void selfUserListItemChanged(class QListWidgetItem *curr, class QListWidgetItem *prev);
    void sendMessage();

private:
    Ui::MainWindow *ui;

    std::unordered_map<char *, class QListWidget> derp;

    class Connection *conn;
};

#endif // MAINWINDOW_H
