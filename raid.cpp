 #include "raid.h"
#include "ui_raid.h"
#include "mainwindow.h"
#include "connection.h"
#include <QDebug>
#include <algorithm>

#define CHARSET_SIZE 27

Raid::Raid(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Raid)
{
    ui->setupUi(this);
    setNNames("");

    connect(ui->prefix, SIGNAL(textChanged(QString)), this, SLOT(setNNames(QString)));
    connect(ui->suffix, SIGNAL(textChanged(QString)), this, SLOT(setNNames(QString)));
    connect(ui->length, SIGNAL(textChanged(QString)), this, SLOT(setNNames(QString)));
    connect(this, SIGNAL(accepted()), &raidSched, SLOT(start()));
    connect(&raidSched, SIGNAL(timeout()), this, SLOT(executeRaid()));

    mainwindow = static_cast<MainWindow *>(parent);
    raidSched.setInterval(500);
}

void Raid::setNNames(QString s)
{
    QString str = "";
    qulonglong pLen = ui->prefix->text().size();
    qulonglong sLen = ui->suffix->text().size();
    qulonglong len = ui->length->text().toULongLong();
    pow_s p;

    if(len > 20) {
        ui->range->setText("name size too large");
    }
    else {
        if(pLen + sLen <= 20) {
            p = iPow(CHARSET_SIZE, len - (pLen + sLen));
            if(p.overflow)
                str += "> ";
            str += p.result;
            str += (p.result != "1") ? " possibilities" : " possibility";
            ui->range->setText(str);
        }
        else {
            ui->range->setText("Prefix/Suffix sum too large");
        }
    }
}

Raid::pow_s Raid::iPow(qulonglong base, qulonglong exp)
{
    pow_s ret;
    qulonglong old, accum = 1;

    for(qulonglong i = 0; i < exp; i++) {
        old = accum;
        accum *= base;
        if(accum <= old) {
            ret.overflow = true;
            ret.result = iToString(old);
            return ret;
        }
    }
    ret.overflow = false;
    ret.result = iToString(accum);
    return ret;
}

QString Raid::iToString(qulonglong i) {
    int d;
    QString str = "";

    while(i) {
        d = (int)(i % 10);
        i /= 10;
        str += QString::number(d);
    }
    std::reverse(str.begin(), str.end());
    return str;
}

void Raid::executeRaid() {
     char *name = new char[21];
     QString pText = ui->prefix->text();
     QString sText = ui->suffix->text();
     std::string stdPText = pText.toStdString();
     std::string stdSText = sText.toStdString();
     const char *prefix = stdPText.c_str();
     const char *suffix = stdSText.c_str();
     ushort plen = stdPText.length();
     ushort slen = stdSText.length();
     ushort len = ui->length->text().toUShort();

     if(len > 20) {
        raidSched.stop();
     }
     else {
         ushort rlen = len - (plen + slen);
         int server = mainwindow->currServerIndex();

         strcpy(name, prefix);
         Connection *c = new Connection(server, mainwindow, NULL);
         Connection::randName(&name[plen], rlen);
         strcpy(&name[plen+rlen], suffix);
         Connection::createAccount(name, "derp");
         c->login(name, "derp");
     }
}

Raid::~Raid()
{
    delete ui;
}
