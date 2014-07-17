#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection.h"
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
    QListWidgetItem *item = new QListWidgetItem;

    item->setBackground(u->color);
    item->setText(u->name);
    ui->userList->addItem(item);

    u->conn->server->insertUser(u);
}

void MainWindow::newSelf(class User *u)
{
    ui->selfUserList->addItem(u->name);
}

MainWindow::~MainWindow()
{
    delete ui;
}
