#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>

int main( int argc, char** argv )
{
    QGuiApplication app( argc, argv );

    QQuickView view;
    view.setResizeMode( QQuickView::SizeRootObjectToView );
    view.setSource( QUrl( "app/native/qml/main.qml" ) );

    QObject::connect( view.engine(), SIGNAL( quit() ),
                      QCoreApplication::instance(), SLOT( quit() ) );
    view.show();

    return app.exec();
}
