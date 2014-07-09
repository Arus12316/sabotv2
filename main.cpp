#include "mainwindow.h"
#include "connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    char buf[21];

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    Connection c(S_2D_CENTRAL);

 /*   for(int i = 0; i < 100; i++) {
        c.randName(buf, 20);
        printf("%s\n", buf);
    }*/

    Connection::randEmail(buf, sizeof buf - 1);

    printf("%lu: %s\n", strlen(buf), buf);

    return a.exec();
}
