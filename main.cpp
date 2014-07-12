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


    Connection c(TEST_COMP, &w);



    c.login("bot.of.doom", "bot");

    qDebug() << "Logged In" <<endl;

    return a.exec();
}
