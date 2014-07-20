#ifndef LOGINMULTIUSERS_H
#define LOGINMULTIUSERS_H

#include <QDialog>

namespace Ui {
class LoginMultiUsers;
}

class LoginMultiUsers : public QDialog
{
    Q_OBJECT

public:
    explicit LoginMultiUsers(QWidget *parent = 0);
    ~LoginMultiUsers();

private:
    Ui::LoginMultiUsers *ui;
};

#endif // LOGINMULTIUSERS_H
