#include "user.h"
#include "server.h"
#include "mainwindow.h"
#include "connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QNetworkProxy proxy;
    //initialize psrng
    qsrand(time(NULL));

    for(int i = 0; i < N_GAMESERVERS; i++) {
        Server::servers[i] = new Server(i);
        printf("%p\n", Server::servers[i]);
    }

    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(9050);

    QNetworkProxy::setApplicationProxy(proxy);

    qDebug() << "Logged In" <<endl;


    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    w.newTab(Server::servers[1]);


    return a.exec();
}
