// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qt5bakelightsnodeinstanceserver.h"

#if BAKE_LIGHTS_SUPPORTED
#include "createscenecommand.h"
#include "view3dactioncommand.h"

#include "nodeinstanceclientinterface.h"
#include "puppettocreatorcommand.h"

#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QProcess>
#include <QQuickView>
#include <QQuickItem>

#include <private/qquickdesignersupport_p.h>
#include <private/qquick3dviewport_p.h>
#endif

namespace QmlDesigner {

Qt5BakeLightsNodeInstanceServer::Qt5BakeLightsNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
    setSlowRenderTimerInterval(100000000);
    setRenderTimerInterval(100);
}

#if BAKE_LIGHTS_SUPPORTED

Qt5BakeLightsNodeInstanceServer::~Qt5BakeLightsNodeInstanceServer()
{
    cleanup();
}

void Qt5BakeLightsNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    initializeView();
    registerFonts(command.resourceUrl);
    setTranslationLanguage(command.language);
    setupScene(command);
    startRenderTimer();

    // Set working directory to a temporary directory, as baking process creates a file there
    if (m_workingDir.isValid())
        QDir::setCurrent(m_workingDir.path());
}

void Qt5BakeLightsNodeInstanceServer::view3DAction(const View3DActionCommand &command)
{
    switch (command.type()) {
    case View3DActionType::SetBakeLightsView3D: {
        QString id = command.value().toString();
        const QList<ServerNodeInstance> allViews = allView3DInstances();
        for (const auto &view : allViews) {
            if (view.id() == id) {
                m_view3D = qobject_cast<QQuick3DViewport *>(view.internalObject());
                break;
            }
        }

        if (!m_view3D)
            abort(tr("View3D not found: '%1'").arg(id));
        else
            startRenderTimer();
        break;
    }
    default:
        break;
    }
}

void Qt5BakeLightsNodeInstanceServer::startRenderTimer()
{
    if (timerId() != 0)
        killTimer(timerId());

    int timerId = startTimer(renderTimerInterval());

    setTimerId(timerId);
}

void Qt5BakeLightsNodeInstanceServer::bakeLights()
{
    if (!m_view3D) {
        abort(tr("Invalid View3D object set."));
        return;
    }

    QQuick3DLightmapBaker::Callback callback = [this](QQuick3DLightmapBaker::BakingStatus status,
            std::optional<QString> msg, QQuick3DLightmapBaker::BakingControl *) {
        m_callbackReceived = true;
        switch (status) {
        case QQuick3DLightmapBaker::BakingStatus::Progress:
        case QQuick3DLightmapBaker::BakingStatus::Warning:
        case QQuick3DLightmapBaker::BakingStatus::Error: {
            nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::BakeLightsProgress, msg.value_or("")});
            nodeInstanceClient()->flush();
        } break;
        case QQuick3DLightmapBaker::BakingStatus::Cancelled:
            abort(tr("Baking cancelled."));
            break;
        case QQuick3DLightmapBaker::BakingStatus::Complete:
            runDenoiser();
            break;
        default:
            qWarning() << __FUNCTION__ << "Unexpected light baking status received:"
                       << int(status) << msg.value_or("");
            break;
        }
    };

    QQuick3DLightmapBaker *baker = m_view3D->lightmapBaker();
    baker->bake(callback);

    m_bakingStarted = true;
}

void Qt5BakeLightsNodeInstanceServer::cleanup()
{
    m_workingDir.remove();
    if (m_denoiser) {
        if (m_denoiser->state() == QProcess::Running)
            m_denoiser->terminate();
        m_denoiser->deleteLater();
    }
}

void Qt5BakeLightsNodeInstanceServer::abort(const QString &msg)
{
    cleanup();
    nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::BakeLightsAborted, msg});
}

void Qt5BakeLightsNodeInstanceServer::finish()
{
    cleanup();
    nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::BakeLightsFinished, {}});
}

void Qt5BakeLightsNodeInstanceServer::runDenoiser()
{
    // Check if denoiser exists (as it is third party app)
    QString binPath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
#if defined(Q_OS_WIN)
    binPath += "/qlmdenoiser.exe";
#else
    binPath += "/qlmdenoiser";
#endif

    QFileInfo fi(binPath);
    if (!fi.exists()) {
        nodeInstanceClient()->handlePuppetToCreatorCommand(
                    {PuppetToCreatorCommand::BakeLightsProgress,
                     tr("Warning: Denoiser executable not found, cannot denoise baked lightmaps (%1).")
                     .arg(binPath)});
        finish();
        return;
    }

    m_denoiser = new QProcess();

    QObject::connect(m_denoiser, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        m_workingDir.remove();
        nodeInstanceClient()->handlePuppetToCreatorCommand(
                    {PuppetToCreatorCommand::BakeLightsProgress,
                     tr("Warning: An error occurred while running denoiser process!")});
        finish();
    });

    QObject::connect(m_denoiser, &QProcess::finished, this, [this](int exitCode,
                                                                   QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            nodeInstanceClient()->handlePuppetToCreatorCommand(
                        {PuppetToCreatorCommand::BakeLightsProgress,
                         tr("Denoising finished.")});
        } else {
            nodeInstanceClient()->handlePuppetToCreatorCommand(
                        {PuppetToCreatorCommand::BakeLightsProgress,
                         tr("Warning: Denoiser process failed with exit code '%1'!").arg(exitCode)});
        }
        finish();
    });

    nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::BakeLightsProgress,
                 tr("Denoising baked lightmaps...")});

    m_denoiser->setWorkingDirectory(m_workingDir.path());
    m_denoiser->start(binPath, {"qlm_list.txt"});

}

void Qt5BakeLightsNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!rootNodeInstance().holdsGraphical())
        return;

    if (!inFunction) {
        inFunction = true;

        QQuickDesignerSupport::polishItems(quickWindow());

        render();

        inFunction = false;
    }
}

void Qt5BakeLightsNodeInstanceServer::render()
{
    // Render multiple times to make sure everything gets rendered correctly before baking
    if (++m_renderCount == 4) {
        bakeLights();
    } else {
        rootNodeInstance().updateDirtyNodeRecursive();
        renderWindow();
        if (m_bakingStarted) {
            slowDownRenderTimer(); // No more renders needed
            if (!m_callbackReceived)
                abort(tr("No bakeable models detected."));
        }
    }
}
#endif

} // namespace QmlDesigner
