#include <QApplication>
#include <QDeclarativeView>
#include <QObject>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QDeclarativeView view;
#ifdef Q_OS_QNX
    view.setSource(QUrl("app/native/qml/main.qml"));
#else
    view.setSource(QUrl("qml/main.qml"));
#endif
    view.setAttribute(Qt::WA_AutoOrientation, true);
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.showMaximized();

    return a.exec();
}
