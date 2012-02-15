#include <QApplication>
#include "html5applicationviewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Html5ApplicationViewer viewer;
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto); // ORIENTATION
    viewer.showExpanded();
#if 1 // DELETE_LINE
    viewer.loadFile(QLatin1String("html/index.html")); // MAINHTMLFILE
#else // DELETE_LINE
    viewer.loadUrl(QUrl(QLatin1String("http://dev.sencha.com/deploy/touch/examples/kitchensink/"))); // MAINHTMLURL
#endif // DELETE_LINE

    return app.exec();
}
