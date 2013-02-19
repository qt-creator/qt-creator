#include <QApplication>
#include "mainwidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // Set default BB10 style. You can also use bb10dark style
    a.setStyle(QLatin1String("bb10bright"));

    MainWidget w;
    w.showMaximized();

    return a.exec();
}
