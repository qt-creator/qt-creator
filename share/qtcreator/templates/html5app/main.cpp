#include <QtGui/QApplication>
#include "html5applicationviewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Html5ApplicationViewer viewer;
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto); // ORIENTATION
    viewer.showExpanded();
    viewer.loadFile(QLatin1String("html/index.html")); // HTMLFILE
//    viewer.loadUrl(QUrl(QLatin1String("http://dev.sencha.com/deploy/touch/examples/kitchensink/")));

    return app.exec();
}
