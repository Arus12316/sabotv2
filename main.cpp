#include "server.h"
#include "mainwindow.h"
#include "connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QNetworkProxy proxy;
    //initialize psrng
    qsrand(time(NULL));

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    for(int i = 0; i < N_GAMESERVERS; i++)
        Server::servers[i] = new Server(i);

    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(9050);

    QNetworkProxy::setApplicationProxy(proxy);

    qDebug() << "Logged In" <<endl;
    return a.exec();
}
