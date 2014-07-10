#include "mainwindow.h"
#include "connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    char buf[21];

    qsrand(time(NULL));

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    Connection c(S_2D_CENTRAL);


    return a.exec();
}
