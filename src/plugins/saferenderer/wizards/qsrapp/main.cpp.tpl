%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([ 'QtCore/QCoreApplication' ],
                           [ 'QtCore/QCoreApplication' ]) }\

#include <QtSafeRenderer/qsafelayout.h>
#include <QtSafeRenderer/qsafelayoutresourcereader.h>
#include <QtSafeRenderer/statemanager.h>

#if defined(HOST_BUILD)
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#endif

#if defined(USE_OUTPUTVERIFIER)
#include <outputverifier.h>
#include "testverifier.h"
#endif

#include "safewindow.h"
#include "eventhandler.h"

int main(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    static SafeRenderer::QSafeLayoutResourceReader layout("/layoutData/main/main.srl");

#if defined(USE_OUTPUTVERIFIER)
    static SafeRenderer::OutputVerifier outputVerifier;
#if defined(HOST_BUILD)
    //In host environment the TestVerifier must be explicitly created.
    //In OpeWFD adaptation the MISRVerifier instance is created in the SafeWindow adaptation.
    static SafeRenderer::TestVerifier testVerifier(outputVerifier);
#endif
    SafeRenderer::SafeWindow telltaleWindow(layout.size(), SafeRenderer::QSafePoint(0U, 0U), outputVerifier);
#else
    SafeRenderer::SafeWindow telltaleWindow(layout.size(), SafeRenderer::QSafePoint(0U, 0U));
#endif
    static SafeRenderer::StateManager stateManager(telltaleWindow, layout);
    telltaleWindow.requestUpdate(); //Request is required because eventHandler is not running yet.

#if defined(USE_OUTPUTVERIFIER)
    SafeRenderer::EventHandler msgHandler(stateManager, telltaleWindow, outputVerifier);
#else
    SafeRenderer::EventHandler msgHandler(stateManager, telltaleWindow);
#endif

#if defined(HOST_BUILD)
    //Mixing the Qt and Qt Safe Renderer renderers is done here only for demonstration purposes on host, not for production purposes of any kind.
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, qApp,
                     [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            qDebug() << "Failed to start the main.qml";
    }, Qt::QueuedConnection);
    engine.addImportPath(":/imports");
    engine.load(url);
#endif

    msgHandler.handleEvents();

    return 0;
}
