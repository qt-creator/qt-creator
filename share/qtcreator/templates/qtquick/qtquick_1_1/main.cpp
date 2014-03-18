#include "qtquick1applicationviewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QtQuick1ApplicationViewer viewer;
    viewer.addImportPath(QLatin1String("modules")); // ADDIMPORTPATH
    viewer.setOrientation(QtQuick1ApplicationViewer::ScreenOrientationAuto); // ORIENTATION
    viewer.setMainQmlFile(QLatin1String("qrc:///qml/main.qml")); // MAINQML
    viewer.showExpanded();

    return app.exec();
}
