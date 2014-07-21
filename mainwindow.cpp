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
    currServer = NULL;
    connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(loginButtonPressed()));
    connect(ui->password, SIGNAL(returnPressed()), this, SLOT(loginButtonPressed()));

    connect(this, &MainWindow::postMiscMessage,
        [=](Server *server, QString *msg) {
            server->miscView->addItem(*msg);
            server->miscView->scrollToBottom();
            delete msg;
    } );
}

void MainWindow::setConn(class Connection *conn)
{
    this->conn = conn;
}

QListWidget *MainWindow::getMessageView()
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

QListWidget *MainWindow::getMiscView()
{
    return ui->miscView;
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

    currServer = c->server;
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
    QString name, *msg;
    QListWidgetItem *item = new QListWidgetItem;

    QList<QListWidgetItem *> selfList = u->conn->server->selfUserList->findItems(QString::fromStdString(u->name), Qt::MatchExactly);

    if(!selfList.size()) {
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

        msg = new QString('<');
        *msg += u->name;
        *msg += ':';
        *msg += u->id;
        *msg += "> has entered the lobby.";

        emit postMiscMessage(u->conn->server, msg);

    }


    lock.lock();
    cond.wakeOne();
    lock.unlock();
}

void MainWindow::newSelf(class User *u)
{
    Server *server = u->conn->server;

    QVariant *data = new QVariant(QVariant::fromValue<Connection *>(u->conn));
    QListWidgetItem *item = new QListWidgetItem();

    item->setText(u->name);
    item->setData(Qt::UserRole, *data);

    ui->selfUserList->addItem(item);

    if(server->master == u->conn) {
        ui->selfUserList->setCurrentRow(0);
        connect(ui->selfUserList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(selfUserListItemChanged(QListWidgetItem*,QListWidgetItem*)));
        connect(server->messageInput, SIGNAL(returnPressed()), this, SLOT(sendMessage()));
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
    QString post("<");

    post += msg->sender->name;
    if(msg->type == '9' || msg->type == 'P') {
        if(msg->type == 'P')
            post += " : [Private]";
        post += "> ";
        post += msg->body;
        msg->sender->conn->server->messageView->addItem(post);
        msg->sender->conn->server->messageView->scrollToBottom();
    }
    else {
        post += " : ";
        post += msg->type;
        post += "> ";
        post += msg->body;
        msg->sender->conn->server->miscView->addItem(post);
        msg->sender->conn->server->miscView->scrollToBottom();
    }
    delete msg;
}

void MainWindow::userDisconnected(User *u)
{
    Connection *conn = u->conn;
    QString *msg = new QString("<");
    QList<QListWidgetItem *> itemList = conn->server->userList->findItems(QString::fromStdString(u->name), Qt::MatchContains);

    delete itemList.first();

    *msg += u->name;
    *msg += ':';
    *msg += u->id;
    *msg += "> has left the lobby.";

    emit postMiscMessage(conn->server, msg);

    lock.lock();
    conn->server->deleteUser(u->id);
    cond.wakeOne();
    lock.unlock();

}

void MainWindow::selfUserListItemChanged(QListWidgetItem *curr, QListWidgetItem *prev)
{
    Connection *conn;
    qDebug() << "Changed! " << endl;

    QVariant data = curr->data(Qt::UserRole);
    conn = data.value<Connection *>();

    currServer->master = conn;
}

void MainWindow::sendMessage()
{
    QString qmsg;
    std::string smsg;
    Connection *conn = currServer->master;


    //not thread safe! Move code to slot
    qmsg = currServer->messageInput->text();
    smsg = qmsg.toStdString();
    conn->sendMessage(smsg.c_str());
    currServer->messageInput->clear();
}

MainWindow::~MainWindow()
{
    delete ui;
}
