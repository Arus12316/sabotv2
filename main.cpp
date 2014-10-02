#include "user.h"
#include "server.h"
#include "mainwindow.h"
#include "connection.h"
#include "regex.h"
#include <stdio.h>
#include "database.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QNetworkProxy proxy;
    //initialize psrng
    qsrand(time(NULL));

    //for C salt generator
    srand(time(NULL));

    for(int i = 0; i < N_GAMESERVERS; i++) {
        Server::servers[i] = new Server(i, NULL);
    }
//142.177.161.27:48900
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("142.177.161.27");
    proxy.setPort(48900);

    QNetworkProxy::setApplicationProxy(proxy);

    regex_s *regex = compile_regex("ab?aa");

    print_nfa(regex->nfa);

    //return 0;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
