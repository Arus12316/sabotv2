#include "loginmultiusers.h"
#include "ui_loginmultiusers.h"

LoginMultiUsers::LoginMultiUsers(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginMultiUsers)
{
    ui->setupUi(this);
}

LoginMultiUsers::~LoginMultiUsers()
{
    delete ui;
}
