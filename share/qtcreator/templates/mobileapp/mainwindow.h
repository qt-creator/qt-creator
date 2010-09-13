#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

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

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
