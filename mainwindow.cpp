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
    const char *username, *password;
    int server = ui->serverList->currentIndex();
    Connection *c = new Connection(server, this);

    username = ui->userName->text().toStdString().c_str();
    password = ui->password->text().toStdString().c_str();

    c->login(username, password);
}


MainWindow::~MainWindow()
{
    delete ui;
}
