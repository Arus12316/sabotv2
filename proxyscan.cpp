#include "proxyscan.h"
#include "server.h"
#include "ui_proxyscan.h"
#include <QFileDialog>
#include <QTest>
#include <QDebug>
#include <QTableWidget>

#define NTHREADS 20

const quint16 ProxyWorker::TIMEOUT = 5000;
const quint16 ProxyWorker::PORT = 1138;
const char *ProxyWorker::testServer = Server::S_2D_CENTRAL;

ProxyWorker::ProxyWorker(QNetworkProxy &proxy, QTableWidgetItem *item, ProxyScan *scan, QObject *parent) :
    proxy(proxy),
    QObject(parent)
{
    this->scan = scan;
    this->item = item;
    connect(&thread, SIGNAL(started()), this, SLOT(tryConnect()));

    this->moveToThread(&thread);

    thread.start();
}

void ProxyWorker::tryConnect()
{
    QTcpSocket sock;

    scan->sem.acquire();
    connect(this, SIGNAL(result(ProxyWorker *, QString)), scan, SLOT(result(ProxyWorker *, QString)));
    sock.setProxy(proxy);
    sock.connectToHost(testServer, PORT, QTcpSocket::ReadWrite);

    if(sock.waitForConnected(TIMEOUT)) {
        emit result(this, "Success");
    }
    else {
        emit result(this, "fail");
    }

    sock.close();
    scan->sem.release();
    emit scan->sigReady();
    thread.exit(0);
}

ProxyScan::ProxyScan(QWidget *parent) :
    sem(NTHREADS),
    QDialog(parent),
    ui(new Ui::ProxyScan)
{
    ui->setupUi(this);
    connect(ui->loadFile, SIGNAL(clicked()), this, SLOT(openFile()));
    connect(this, SIGNAL(sigReady()), this, SLOT(scan()));
}

void ProxyScan::scan()
{
    char c;
    QString key;

    while(sem.available() && file->getChar(&c)) {
        if(c == '\n') {
            parseLine(key);
            key.clear();
        }
        else {
            key += c;
        }
    }
}

void ProxyScan::parseLine(QString &line)
{
    bool ok;
    QNetworkProxy *proxy;
    QTableWidgetItem *status;
    QStringList tokens = line.split(":"), ipDigits;
    QString ip, portStr;
    quint16 port, digit;

    if(tokens.size() >= 2) {
        ip = tokens[0];
        ipDigits = ip.split(".");

        if(ipDigits.size() == 4) {
            foreach(const QString &dig, ipDigits) {
                digit = dig.toUShort(&ok);
                if(digit > 255 || !ok) {
                    return;
                }
            }
            portStr = tokens[1];
            port = portStr.toUShort(&ok);
            if(!ok) {
                return;
            }
            status = addProxy(ip, portStr);
            proxy = new QNetworkProxy(QNetworkProxy::Socks5Proxy, ip, port);
            new ProxyWorker(*proxy, status, this);
        }
    }
}

QTableWidgetItem *ProxyScan::addProxy(QString &host, QString &port)
{
    int rowCount;
    QString *proxyStr;
    QTableWidget *table;
    QTableWidgetItem *proxy, *status;

    proxyStr = new QString(host);
    *proxyStr += ":";
    *proxyStr += port;
    proxy = new QTableWidgetItem(*proxyStr);
    status = new QTableWidgetItem("Connecting...");
    table = ui->proxyTable;
    rowCount = table->rowCount();
    table->insertRow(rowCount);
    table->setItem(rowCount, 0, proxy);
    table->setItem(rowCount, 1, status);
    return status;
}

void ProxyScan::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Files (*)"));
    file = new QFile(fileName, this);
    file->open(QIODevice::ReadOnly);
    if(!file->isOpen()) {
        qDebug() << "Error Opening File";
    }
    scan();
    raise();
}

void ProxyScan::result(ProxyWorker *src, QString res)
{
    src->item->setText(res);
}

ProxyScan::~ProxyScan()
{
    delete ui;
}
