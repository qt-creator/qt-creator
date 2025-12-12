%{Cpp:LicenseTemplate}\
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
#include <outputverifier_capi.h>
#endif

#include "safewindow.h"
#include "eventhandler.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    static SafeRenderer::QSafeLayoutResourceReader layout("/layoutData/main/main.srl");

    SafeRenderer::SafeWindow telltaleWindow(layout.size(), SafeRenderer::QSafePoint(0U, 0U));

#if defined(USE_OUTPUTVERIFIER)
    SafeRenderer::OutputVerifier &outputVerifier = SafeRenderer::OutputVerifier::getOutputVerifierInstance();
    OutputVerifier_initVerifierDevice(0, 0, layout.size().width(), layout.size().height());
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
