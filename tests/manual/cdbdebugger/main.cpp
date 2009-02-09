#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainWindow mw;
    if (argc >= 2) mw.setDebuggee(argv[1]);
    mw.show();
    return app.exec();
}
