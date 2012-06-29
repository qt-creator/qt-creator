#include <QtGui/QGuiApplication>
#include "mainwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWidget w;
    w.showFullScreen();


    return a.exec();
}
