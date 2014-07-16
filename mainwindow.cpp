#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection.h"
#include "server.h"

#include <QTabWidget>

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

void MainWindow::newTab(const char *server)
{

}

void MainWindow::loginButtonPressed()
{
    QString username, password;
    int server = ui->serverList->currentIndex();
    Connection *c = new Connection(server, this);

    username = ui->userName->text();
    password = ui->password->text();

    c->login(username.toStdString().c_str(), password.toStdString().c_str());
    ui->userName->clear();
    ui->password->clear();
}


MainWindow::~MainWindow()
{
    delete ui;
}
