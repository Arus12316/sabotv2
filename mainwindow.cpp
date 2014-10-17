#include "mainwindow.h"
#include "connection.h"
#include "raid.h"
#include "ui_mainwindow.h"
#include "user.h"
#include "server.h"
#include "proxyscan.h"
#include "createaccount.h"
#include <QtTest/QTest>
#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    db(),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    currServer = NULL;
    ca = NULL;
    prox = NULL;
    raidDialog = NULL;

    connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(loginButtonPressed()));
    connect(ui->password, SIGNAL(returnPressed()), this, SLOT(loginButtonPressed()));
    connect(ui->createAccountButton, SIGNAL(clicked()), this, SLOT(createAccount()));
    connect(ui->proxyScanButton, SIGNAL(pressed()), this, SLOT(openProxyScan()));
    connect(ui->selfUserList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(selfUserListItemChanged(QListWidgetItem*,QListWidgetItem*)));

    connect(ui->raidButton, SIGNAL(pressed()), this, SLOT(raid()));

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

QLineEdit *MainWindow::getPmAutoReply()
{
    return ui->autoReply;
}

bool MainWindow::calculatorOn()
{
    return ui->calculatorOn->isChecked();
}

bool MainWindow::calculatorPM()
{
    return ui->calculatorPM->isChecked();
}

bool MainWindow::autoReconnectIsChecked()
{
    return ui->autoReconnect->isChecked();
}

void MainWindow::newTab(const char *server)
{

}

void MainWindow::qStrCpy(char *dst, QString &src)
{
    std::string stdstr = src.toStdString();
    const char *cstr = stdstr.c_str();
    strcpy(dst, cstr);
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
    QString name, *msg, *floodMSG;
    QListWidgetItem *item;

    time_t t = time(NULL);

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
        name += "   ";
    name += u->name;
    item->setBackground(u->color);
    item->setText(name);
    ui->userList->addItem(item);
    u->conn->server->insertUser(u);

    switch(Connection::checkFlood(*currServer->cap)) {
        case FLOOD_END:
            floodMSG = new QString("<<User c/dc Flood off>>");
            emit postMiscMessage(u->conn->server, floodMSG);
        case FLOOD_NONE:
            msg = new QString('<');
            *msg += u->name;
            *msg += ':';
            *msg += u->id;
            *msg += "> has entered the lobby.";
            emit postMiscMessage(u->conn->server, msg);
            break;
        case FLOOD_START:
            floodMSG = new QString("<<User c/dc Flood on>>");
            emit postMiscMessage(u->conn->server, floodMSG);
        case FLOOD_STILL:
            break;
    }

    lock.lock();
    cond.wakeOne();
    lock.unlock();

    qDebug() << "Diff: " << time(NULL) - t;
}

void MainWindow::newSelf(UserSelf *u)
{
    Server *server = u->conn->server;
    static int bob;

    qDebug() << "Called newSelf";

    QVariant *data = new QVariant(QVariant::fromValue<Connection *>(u->conn));
    QListWidgetItem *item = new QListWidgetItem();

    item->setText(u->name);
    item->setData(Qt::UserRole, *data);

    ui->selfUserList->addItem(item);
    u->selfEntry = item;
    if(server->master == u->conn && !bob) {
        bob = 1;
        ui->selfUserList->setCurrentRow(0);
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
    message_s *reply;
    std::string stdrep;
    const char *crep;
    QString autoReply, original;
    QString post("<");
    QListWidget *list;
    QScrollBar *scroll;
    //time_t t = time(NULL);

    post += msg->sender->name;
    if(msg->type == '9' || msg->type == 'P') {
        list = msg->sender->conn->server->messageView;
        scroll = list->verticalScrollBar();
        if(msg->type == 'P') {
            original = ui->autoReply->text();
            if(original.length()) {
                reply = new message_s;
                reply->type = 'P';
                reply->receiver = msg->sender;
                autoReply = original.replace('%', msg->body);
                autoReply = autoReply.replace('&', msg->sender->name);
                stdrep = autoReply.toStdString();
                crep = stdrep.c_str();
                strncpy(reply->body, crep, MAX_MSGLEN-1);
                qDebug() << "Sending Response: " << reply->body;
                emit sendPrivateMessage(reply);
            }

            post += " : [Private]";
        }
        post += "> ";
        post += msg->body;

        list->addItem(post);
        if(scroll->maximum() == scroll->sliderPosition())
            list->scrollToBottom();
    }
    else {
        post += " : ";
        post += msg->type;
        post += "> ";
        post += msg->body;
        list = msg->sender->conn->server->miscView;
        scroll = list->verticalScrollBar();
        list->addItem(post);
        if(scroll->maximum() == scroll->sliderPosition())
            list->scrollToBottom();

    }
    //qDebug() << "time: " << time(NULL) - t;

    delete msg;
}

int MainWindow::currServerIndex()
{
    return ui->serverList->currentIndex();
}

void MainWindow::userDisconnected(User *u)
{
    Connection *conn = u->conn;
    QString *msg = new QString("<"), *floodMSG;

    qDebug() << endl << u->name << " : " << u->id << " disconnected." << endl;

    switch(Connection::checkFlood(*currServer->cap)) {
        case FLOOD_END:
            floodMSG = new QString("<<User c/dc Flood off>>");
            emit postMiscMessage(u->conn->server, floodMSG);
        case FLOOD_NONE:
            *msg += u->name;
            *msg += ':';
            *msg += u->id;
            *msg += "> has left the lobby.";
            emit postMiscMessage(conn->server, msg);
            break;
        case FLOOD_START:
            floodMSG = new QString("<<User c/dc Flood on>>");
            emit postMiscMessage(u->conn->server, floodMSG);
        case FLOOD_STILL:
            break;
    }

    lock.lock();
    conn->server->deleteUser(u->id);
    cond.wakeOne();
    lock.unlock();
}

void MainWindow::selfUserListItemChanged(QListWidgetItem *curr, QListWidgetItem *prev)
{
    Connection *connPrev, *connCurr;

    qDebug() << "Called selfUserListItemChanged";

    if(curr != NULL && prev != NULL) {
        qDebug() << "curr and prev are not null";
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
    else if(prev != NULL) {
        qDebug() << "prev is not null";
        QVariant dataPrev = prev->data(Qt::UserRole);
        connPrev = dataPrev.value<Connection *>();

        disconnect(this, SIGNAL(sendPublicMessage(QString *)), connPrev, SLOT(sendPublicMessage(QString *)));
        disconnect(this, SIGNAL(sendPrivateMessage(message_s *)), connPrev, SLOT(sendPrivateMessage(message_s *)));
        disconnect(this, SIGNAL(sendRawMessage(QString)), connPrev, SLOT(sendRaw(QString)));
        //currServer->currConn = NULL;
    }
    else if(curr != NULL) {
        /*qDebug() << "Curr is not null";
        QVariant dataCurr = curr->data(Qt::UserRole);
        connCurr = dataCurr.value<Connection *>();

        currServer->currConn = conn;
        connect(this, SIGNAL(sendPublicMessage(QString *)), connCurr, SLOT(sendPublicMessage(QString *)));
        connect(this, SIGNAL(sendPrivateMessage(message_s *)), connCurr, SLOT(sendPrivateMessage(message_s *)));
        connect(this, SIGNAL(sendRawMessage(QString)), connCurr, SLOT(sendRaw(QString)));*/
    }
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
    if(!raidDialog)
        raidDialog = new Raid(this);
    raidDialog->show();
    raidDialog->raise();
    raidDialog->activateWindow();
}

void MainWindow::loginRecover(Connection *last)
{
    int serverIndex;
    Connection *rec;
    char oldName[32], oldPass[32];
    QString recMsg = "[ Attempting to Recover Connection for: ";

    recMsg += last->username;
    recMsg += " ]";
    ui->messageView->addItem(recMsg);

    for(serverIndex = 0; serverIndex < N_GAMESERVERS; serverIndex++) {
        if(Server::servers[serverIndex] == last->server)
            break;
    }

    if(last == last->server->master)
        last->server->master = NULL;

    userDisconnected(last->user);
    if(!ui->selfUserList->count()) {
        ui->userList->clear();
    }
    last->server->master = NULL;
    strcpy(oldName, last->username);
    strcpy(oldPass, last->password);

    delete last;

    rec = new Connection(serverIndex, this, NULL);
    conn = rec;
    currServer = rec->server;
    rec->login(oldName, oldPass);
}

void MainWindow::openProxyScan()
{
    if(!prox)
        prox = new ProxyScan(this);
    prox->show();
    prox->raise();
    prox->activateWindow();
}

MainWindow::~MainWindow()
{
    delete ui;
}
