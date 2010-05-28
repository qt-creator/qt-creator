#include "batteryindicator.h"
#include "ui_batteryindicator.h"

//! [2]
BatteryIndicator::BatteryIndicator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatteryIndicator),
    deviceInfo(NULL)
{
    ui->setupUi(this);
    setupGeneral();
}
//! [2]

BatteryIndicator::~BatteryIndicator()
{
    delete ui;
}

//! [1]
void BatteryIndicator::setupGeneral()
{
    deviceInfo = new QSystemDeviceInfo(this);

    ui->batteryLevelBar->setValue(deviceInfo->batteryLevel());

    connect(deviceInfo, SIGNAL(batteryLevelChanged(int)),
            ui->batteryLevelBar, SLOT(setValue(int)));
}
//! [1]
