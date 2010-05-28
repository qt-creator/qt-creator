#ifndef BATTERYINDICATOR_H
#define BATTERYINDICATOR_H

#include <QDialog>

//! [1]
#include <QSystemInfo>
//! [1]

//! [2]
QTM_USE_NAMESPACE
//! [2]

namespace Ui {
    class BatteryIndicator;
}

class BatteryIndicator : public QDialog
{
    Q_OBJECT

public:
    explicit BatteryIndicator(QWidget *parent = 0);
    ~BatteryIndicator();

//! [3]
private:
    Ui::BatteryIndicator *ui;
    void setupGeneral();

    QSystemDeviceInfo *deviceInfo;
//! [3]
};

#endif // BATTERYINDICATOR_H
