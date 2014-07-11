#include "mainwindow.h"
#include "connection.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    //initialize psrng
    qsrand(time(NULL));

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    Connection c(TEST_COMP, &w);

    c.login("bot.of.doom", "bot");
    printf("%lu\n", sizeof c);

    return a.exec();
}
