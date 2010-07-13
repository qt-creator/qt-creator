#include <QtGui/QApplication>
#include "batteryindicator.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BatteryIndicator w;

//! [0]
#if defined(Q_WS_S60)
    w.showMaximized();
#else
    w.show();
#endif
//! [0]

    return a.exec();
}
