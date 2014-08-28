#ifndef RAID_H
#define RAID_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class Raid;
}

class Raid : public QDialog
{
    Q_OBJECT

public:
    explicit Raid(QWidget *parent);
    ~Raid();

public slots:
    void setNNames(QString);
    void executeRaid();

private:
    struct pow_s {
        QString result;
        bool overflow;
    };

    pow_s iPow(qulonglong base, qulonglong exp);
    QString iToString(qulonglong i);
    class MainWindow *mainwindow;
    Ui::Raid *ui;
    class QTimer raidSched;
};

#endif // RAID_H
