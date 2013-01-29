#include <QGuiApplication>
#include "qtquick2applicationviewer.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QtQuick2ApplicationViewer viewer;
    viewer.setMainQmlFile(QLatin1String("qml/main.qml")); // MAINQML
    viewer.showExpanded();

    return app.exec();
}