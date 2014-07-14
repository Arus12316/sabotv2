#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->inputMessage, SIGNAL(returnPressed()), this, SLOT(pressEnter()));
}

void MainWindow::setConn(class Connection *conn)
{
    this->conn = conn;
}


void MainWindow::slotConnectedMsg()
{
}

void MainWindow::pressEnter()
{
    conn->sendMessage(ui->inputMessage->text().toStdString().c_str());
    qDebug() << "Sending: " << ui->inputMessage->text().toStdString().c_str();
}


MainWindow::~MainWindow()
{
    delete ui;
}
