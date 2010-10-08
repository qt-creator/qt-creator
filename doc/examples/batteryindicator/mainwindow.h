// checksum 0xa940 version 0x10001
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

//! [1]
#include <QSystemInfo>
//! [1]

//! [2]
QTM_USE_NAMESPACE
//! [2]

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
public:
    enum Orientation {
        LockPortrait,
        LockLandscape,
        Auto
    };

    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setOrientation(Orientation orientation);

//! [3]
private:
    Ui::MainWindow *ui;
    void setupGeneral();

    QSystemDeviceInfo *deviceInfo;
//! [3]
};

#endif // MAINWINDOW_H
