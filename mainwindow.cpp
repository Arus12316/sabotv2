#include "mainwindow.h"
#include "connection.h"
#include "ui_mainwindow.h"
#include "user.h"
#include "server.h"

#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(loginButtonPressed()));
    connect(ui->password, SIGNAL(returnPressed()), this, SLOT(loginButtonPressed()));
}

void MainWindow::setConn(class Connection *conn)
{
    this->conn = conn;
}

QListWidget *MainWindow::getMessageBox()
{
    return ui->messageView;
}

QListWidget *MainWindow::getUserList()
{
    return ui->userList;
}

QLineEdit *MainWindow::getMessageInput()
{
    return ui->messageInput;
}

QListWidget *MainWindow::getSelfUserList()
{
    return ui->selfUserList;
}

void MainWindow::newTab(const char *server)
{

}

void MainWindow::loginButtonPressed()
{
    QString username, password;
    std::string stdUsername, stdPassword;
    int server = ui->serverList->currentIndex();
    Connection *c = new Connection(server, this);

    username = ui->userName->text();
    password = ui->password->text();

    stdUsername = username.toStdString();
    stdPassword = password.toStdString();

    c->login(stdUsername.c_str(), stdPassword.c_str());
    ui->userName->clear();
    ui->password->clear();
}

void MainWindow::newUser(User *u)
{
    QString name;
    QListWidgetItem *item = new QListWidgetItem;

    if(u->modLevel > '0') {
        name += "M";
        name += u->modLevel;
        name += ' ';
    }
    else
        name += "     ";
    name += u->name;
    item->setBackground(u->color);
    item->setText(name);
    ui->userList->addItem(item);

    u->conn->server->insertUser(u);
}

void MainWindow::newSelf(class User *u)
{
    ui->selfUserList->addItem(u->name);
    if(u->conn->server->master == u->conn) {
        ui->selfUserList->setCurrentRow(0);
    }
}

void MainWindow::newTab(Server *server)
{
    if(ui->tabs->tabText(0) == "<not connected>") {
        ui->tabs->setTabText(0, server->getName());
    }
    else {
        QWidget *prototype = ui->tabs->widget(0);
        QString name("server");
        QWidget *layout = new QWidget(ui->tabs);

        QListWidget *selfUserList;
        QListWidget *userList;

        name += ui->tabs->count() + 1;
        layout->setObjectName(name);

        selfUserList = new QListWidget(layout);
        selfUserList->move(ui->selfUserList->pos());
        selfUserList->resize(ui->selfUserList->size());

        userList = new QListWidget(layout);
        userList->move(ui->userList->pos());
        userList->resize(ui->userList->size());

        ui->tabs->addTab(layout, server->getName());
    }
}

void MainWindow::postMessage(message_s *msg)
{
    QString post ;

    post += "<";
    post += msg->sender->name;
    if(msg->type == 'P')
        post += " : P";
    post += "> ";
    post += msg->body;

    msg->view->messageBox->addItem(post);

    delete msg;
}

void MainWindow::deleteUser(Connection *conn, char *id)
{
    User *user = conn->server->lookupUser(id);
    QList<QListWidgetItem *> itemList = conn->server->userList->findItems(QString::fromStdString(user->name), Qt::MatchContains);

    qDebug() << endl << user->name << " has disconnected" << endl;
    delete itemList.first();
    conn->server->deleteUser(id);

    delete[] id;
    delete user;
}

MainWindow::~MainWindow()
{
    delete ui;
}
