#include <QApplication>
#include "qmlapplicationviewer.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QApplication> app(createApplication(argc, argv));

    QmlApplicationViewer viewer;
    viewer.addImportPath(QLatin1String("modules")); // ADDIMPORTPATH
    viewer.setOrientation(QmlApplicationViewer::ScreenOrientationAuto); // ORIENTATION
    viewer.setMainQmlFile(QLatin1String("qml/main.qml")); // MAINQML
    viewer.showExpanded();

    return app->exec();
}
