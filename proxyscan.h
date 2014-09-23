#ifndef PROXYSCAN_H
#define PROXYSCAN_H
#include <QHash>
#include <QDialog>
#include <QThread>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QTableWidgetItem>
#include <QSemaphore>

class ProxyWorker : public QObject
{
    Q_OBJECT

public:
    QTableWidgetItem *item;

    explicit ProxyWorker(QNetworkProxy &proxy, QTableWidgetItem *item, class ProxyScan *scan, QObject *parent = 0);

signals:
    void result(ProxyWorker *src, QString res);

private slots:
    void tryConnect();

private:
    QThread thread;
    QNetworkProxy &proxy;
    QTcpSocket sock;
    class ProxyScan *scan;

    static const quint16 TIMEOUT;
    static const quint16 PORT;
    static const char *testServer;
};

namespace Ui {
class ProxyScan;
}

class ProxyScan : public QDialog
{
    Q_OBJECT

public:
    QSemaphore sem;
    QHash<ProxyWorker *, QTableWidgetItem *> record;

    explicit ProxyScan(QWidget *parent = 0);
    ~ProxyScan();

    void parseLine(QString &line);

signals:
    void sigReady();

public slots:
    void scan();


    QTableWidgetItem *addProxy(QString &host, QString &port);

public slots:
    void openFile();
    void result(ProxyWorker *src, QString res);

private:
    Ui::ProxyScan *ui;
    class QFile *file;
};

#endif // PROXYSCAN_H
