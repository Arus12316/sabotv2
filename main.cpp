#include "mainwindow.h"
#include "connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{

    char buf[21];
    //initialize psrng
    qsrand(time(NULL));

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    Connection c(TEST_FW, &w);

    c.login("bot.of.doom", "bot");

    qDebug() << "Logged In" <<endl;

    return a.exec();
}
