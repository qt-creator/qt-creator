#include <QtGui/QApplication>
#include "qmlapplicationviewer.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QApplication> app(createApplication(argc, argv));
    QScopedPointer<QmlApplicationViewer> viewer(QmlApplicationViewer::create());

    viewer->addImportPath(QLatin1String("modules")); // ADDIMPORTPATH
    viewer->setOrientation(QmlApplicationViewer::ScreenOrientationAuto); // ORIENTATION
    viewer->setMainQmlFile(QLatin1String("qml/app/main.qml")); // MAINQML
    viewer->showExpanded();

    return app->exec();
}
