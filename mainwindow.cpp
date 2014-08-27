#include "mainwindow.h"
#include "connection.h"
#include "ui_mainwindow.h"
#include "user.h"
#include "server.h"
#include "createaccount.h"
#include <QtTest/QTest>
#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    currServer = NULL;
    ca = NULL;
    raidSched.setInterval(500);

    connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(loginButtonPressed()));
    connect(ui->password, SIGNAL(returnPressed()), this, SLOT(loginButtonPressed()));
    connect(ui->createAccountButton, SIGNAL(clicked()), this, SLOT(createAccount()));


    connect(ui->raidButton, SIGNAL(pressed()), &raidSched, SLOT(start()));
    connect(&raidSched, SIGNAL(timeout()), this, SLOT(raid()));

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

QListWidget *MainWindow::getGameView()
{
    return ui->gameList;
}

QPushButton *MainWindow::getSendButton()
{
    return ui->sendButton;
}

QPushButton *MainWindow::getPmButton()
{
    return ui->pmButton;
}

QLabel *MainWindow::getCurrUserLabel()
{
    return ui->currUserLabel;
}

QLineEdit *MainWindow::getInputRaw()
{
    return ui->inputRaw;
}

void MainWindow::newTab(const char *server)
{

}

void MainWindow::loginButtonPressed()
{
    QString username, password;
    std::string stdUsername, stdPassword;
    int server = ui->serverList->currentIndex();
    Connection *c = new Connection(server, this, NULL);

    username = ui->userName->text();
    password = ui->password->text();

    if(!currServer) {
        ui->currUserLabel->setText(username);
    }

    currServer = c->server;

    stdUsername = username.toStdString();
    stdPassword = password.toStdString();

    c->login(stdUsername.c_str(), stdPassword.c_str());
    ui->userName->clear();
    ui->password->clear();
}

void MainWindow::newUser(User *u)
{
    QVariant *data;
    QString name, *msg;
    QListWidgetItem *item;
    QList<QListWidgetItem *> selfList = u->conn->server->selfUserList->findItems(QString::fromStdString(u->name), Qt::MatchExactly);

    if(!selfList.size()) {
        item = new QListWidgetItem;
        u->listEntry = item;
        data = new QVariant(QVariant::fromValue<User *>(u));
        item->setData(Qt::UserRole, *data);

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
        connect(server->messageInput, SIGNAL(returnPressed()), this, SLOT(preparePublicMessage()));
        connect(server->sendButton, SIGNAL(clicked()), this, SLOT(preparePublicMessage()));
        connect(server->pmButton, SIGNAL(clicked()), this, SLOT(preparePrivateMessage()));
        connect(server->inputRaw, SIGNAL(returnPressed()), this, SLOT(prepareRawMessage()));
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

    qDebug() << endl << u->name << " : " << u->id << " disconnected." << endl;

    delete u->listEntry;

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
    Connection *connPrev, *connCurr;
    qDebug() << "Changed! " << endl;

    QVariant dataCurr = curr->data(Qt::UserRole);
    QVariant dataPrev = prev->data(Qt::UserRole);
    connPrev = dataPrev.value<Connection *>();
    connCurr = dataCurr.value<Connection *>();

    disconnect(this, SIGNAL(sendPublicMessage(QString *)), connPrev, SLOT(sendPublicMessage(QString *)));
    disconnect(this, SIGNAL(sendPrivateMessage(message_s *)), connPrev, SLOT(sendPrivateMessage(message_s *)));
    disconnect(this, SIGNAL(sendRawMessage(QString)), connPrev, SLOT(sendRaw(QString)));
    currServer->currConn = conn;
    connect(this, SIGNAL(sendPublicMessage(QString *)), connCurr, SLOT(sendPublicMessage(QString *)));
    connect(this, SIGNAL(sendPrivateMessage(message_s *)), connCurr, SLOT(sendPrivateMessage(message_s *)));
    connect(this, SIGNAL(sendRawMessage(QString)), connCurr, SLOT(sendRaw(QString)));

    ui->currUserLabel->setText(curr->text());
}

void MainWindow::preparePublicMessage()
{
    QString *msg;
    Connection *conn = currServer->currConn;

    msg = new QString(currServer->messageInput->text());

    emit sendPublicMessage(msg);

    currServer->messageInput->clear();
}

void MainWindow::preparePrivateMessage()
{
    QVariant uData = currServer->userList->currentItem()->data(Qt::UserRole);
    User *u = uData.value<User *>();

    QString qtext = currServer->messageInput->text();
    std::string stdText = qtext.toStdString();
    const char *cstr = stdText.c_str();

    message_s *msg = new message_s;
    strcpy(msg->body, cstr);
    msg->receiver = u;
    msg->type = 'P';

    emit sendPrivateMessage(msg);

    QString post("<< pm to: ");

    post += u->name;
    post += ">> ";
    post += cstr;

    currServer->messageView->addItem(post);
    currServer->messageView->scrollToBottom();
}

void MainWindow::prepareRawMessage()
{
    QString post("<--sent raw--> ");
    QString msg = ui->inputRaw->text();

    post += msg;

    emit sendRawMessage(msg);

    currServer->messageView->addItem(post);
    currServer->messageView->scrollToBottom();
}


void MainWindow::postGameList(Connection *conn)
{
    conn->server->gameView->clear();
    conn->gameList.pop_front();
    conn->server->gameView->addItems(conn->gameList);
    conn->gameList.clear();

    conn->listLock.lock();
    conn->listCond.wakeOne();
    conn->listLock.unlock();
}

void MainWindow::postGeneralMain(Server *server, QString msg)
{
    server->messageView->addItem(msg);
    server->messageView->scrollToBottom();
}

void MainWindow::postGeneralMisc(Server *server, QString msg)
{
    server->miscView->addItem(msg);
    server->miscView->scrollToBottom();
}

void MainWindow::createAccount()
{
    if(!ca)
        ca = new CreateAccount(this);
    ca->show();
    ca->raise();
    ca->activateWindow();
}

void MainWindow::raid()
{
    char *name;
    int server = ui->serverList->currentIndex();

    for(int i = 0; i < 1; i++) {
        name = new char[20];
        Connection *c = new Connection(server, this, NULL);
        Connection::randName(name, 15);
        Connection::createAccount(name, "derp");
        c->login(name, "derp");
    }
}

void MainWindow::loginRecover(Connection *last)
{
    int serverIndex;
    Connection *rec;

    for(serverIndex = 0; serverIndex < N_GAMESERVERS; serverIndex++) {
        if(Server::servers[serverIndex] == last->server)
            break;
    }

    if(last == last->server->master)
        last->server->master = NULL;

    userDisconnected(last->user);

    rec = new Connection(serverIndex, this, NULL);

    rec->login(last->username, last->password);
    last->deleteLater();
}

MainWindow::~MainWindow()
{
    delete ui;
}
