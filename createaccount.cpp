#include "createaccount.h"
#include "connection.h"
#include "ui_createaccount.h"

CreateAccount::CreateAccount(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateAccount)
{

    connect(this, SIGNAL(accepted()), this, SLOT(createAccount()));

    ui->setupUi(this);
}

void CreateAccount::createAccount()
{

    QString username = ui->inputUsername->text(),
            password = ui->inputPassword->text();
    std::string stdusr = username.toStdString(),
                stdpss = password.toStdString();
    const char  *cuser = stdusr.c_str(),
                *cpass = stdpss.c_str();

    Connection::createAccount(cuser, cpass);

    ui->inputUsername->clear();
    ui->inputPassword->clear();
}

CreateAccount::~CreateAccount()
{
    delete ui;
}
