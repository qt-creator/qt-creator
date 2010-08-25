#include <QtGui/QApplication>
#include "qmlapplicationviewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QmlApplicationViewer viewer;
    viewer.addImportPath(QLatin1String("modules")); // ADDIMPORTPATH
    viewer.setOrientation(QmlApplicationViewer::Auto); // ORIENTATION
    viewer.setMainQmlFile(QLatin1String("qml/app/app.qml")); // MAINQML
    viewer.setLoadDummyData(false); // LOADDUMMYDATA

#ifdef Q_OS_SYMBIAN
    viewer.showFullScreen();
#elif defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6)
    viewer.showMaximized();
#else
    viewer.show();
#endif
    return app.exec();
}
