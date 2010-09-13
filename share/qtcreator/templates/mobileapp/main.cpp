#include "mainwindow.h"

#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.setOrientation(MainWindow::Auto); // ORIENTATION

#ifdef Q_OS_SYMBIAN
    mainWindow.showFullScreen();
#elif defined(Q_WS_MAEMO_5)
    mainWindow.showMaximized();
#else
    mainWindow.show();
#endif
    return app.exec();
}
