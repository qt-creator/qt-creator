#include <QWindow>
#include <QtDeclarative>
#include <QQuickView>

int main( int argc, char** argv )
{
    QGuiApplication app( argc, argv );

    QWindow *window = 0;
    int exitCode = 0;

    QQuickView* view = new QQuickView();
    view->setResizeMode( QQuickView::SizeRootObjectToView );
    view->setSource( QUrl( "app/native/qml/main.qml" ) );

    QDeclarativeEngine* engine = view->engine();
    QObject::connect( engine, SIGNAL( quit() ),
                      QCoreApplication::instance(), SLOT( quit() ) );
    window = view;
    window->showMaximized();

    exitCode = app.exec();

    delete window;
    return exitCode;
}
