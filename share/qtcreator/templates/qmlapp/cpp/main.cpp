#include <QtGui/QApplication>
#include "qmlapplicationview.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QmlApplicationView qmlApp;
    qmlApp.addImportPath(QLatin1String("modules")); // ADDIMPORTPATH
    qmlApp.setOrientation(QmlApplicationView::Auto); // ORIENTATION
    qmlApp.setMainQml(QLatin1String("qml/app/app.qml")); // MAINQML
    qmlApp.setLoadDummyData(false); // LOADDUMMYDATA

#ifdef Q_OS_SYMBIAN
    qmlApp.showFullScreen();
#elif defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6)
    qmlApp.showMaximized();
#else
    qmlApp.show();
#endif
    return app.exec();
}
