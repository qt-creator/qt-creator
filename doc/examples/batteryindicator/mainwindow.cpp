// checksum 0xd429 version 0x10001
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QCoreApplication>

#if defined(Q_OS_SYMBIAN) && defined(ORIENTATIONLOCK)
#include <eikenv.h>
#include <eikappui.h>
#include <aknenv.h>
#include <aknappui.h>
#endif // Q_OS_SYMBIAN && ORIENTATIONLOCK

//! [2]
MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    deviceInfo(NULL)
{
    ui->setupUi(this);
    setupGeneral();
}
//! [2]

MainWindow::~MainWindow()
{
    delete ui;
}

//! [1]
void MainWindow::setupGeneral()
{
    deviceInfo = new QSystemDeviceInfo(this);

    ui->batteryLevelBar->setValue(deviceInfo->batteryLevel());

    connect(deviceInfo, SIGNAL(batteryLevelChanged(int)),
            ui->batteryLevelBar, SLOT(setValue(int)));
}
//! [1]

void MainWindow::setOrientation(Orientation orientation)
{
#ifdef Q_OS_SYMBIAN
    if (orientation != Auto) {
#if defined(ORIENTATIONLOCK)
        const CAknAppUiBase::TAppUiOrientation uiOrientation =
                (orientation == LockPortrait) ? CAknAppUi::EAppUiOrientationPortrait
                    : CAknAppUi::EAppUiOrientationLandscape;
        CAknAppUi* appUi = dynamic_cast<CAknAppUi*> (CEikonEnv::Static()->AppUi());
        TRAPD(error,
            if (appUi)
                appUi->SetOrientationL(uiOrientation);
        );
#else // ORIENTATIONLOCK
        qWarning("'ORIENTATIONLOCK' needs to be defined on Symbian when locking the orientation.");
#endif // ORIENTATIONLOCK
    }
#elif defined(Q_WS_MAEMO_5)
    Qt::WidgetAttribute attribute;
    switch (orientation) {
    case LockPortrait:
        attribute = Qt::WA_Maemo5PortraitOrientation;
        break;
    case LockLandscape:
        attribute = Qt::WA_Maemo5LandscapeOrientation;
        break;
    case Auto:
    default:
        attribute = Qt::WA_Maemo5AutoOrientation;
        break;
    }
    setAttribute(attribute, true);
#else // Q_OS_SYMBIAN
    Q_UNUSED(orientation);
#endif // Q_OS_SYMBIAN
}
