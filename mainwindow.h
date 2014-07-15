#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>


namespace Ui {
class MainWindow;

}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setConn(class Connection *conn);
    void newTab(const char *server);

public slots:
    void loginButtonPressed();


private:
    Ui::MainWindow *ui;
    class Connection *conn;
};

#endif // MAINWINDOW_H
