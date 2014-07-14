#include "mainwindow.h"
#include "connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{

    QNetworkProxy proxy;
    char buf[21];
    //initialize psrng
    qsrand(time(NULL));

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(9050);

    QNetworkProxy::setApplicationProxy(proxy);

    //Connection::createAccount("immanoooob", "bot");

    Connection c(NULL, TEST_PTC, &w);
    c.login("sexy.little.bot", "bot");

    w.setConn(&c);

    qDebug() << "Logged In" <<endl;

    return a.exec();
}
