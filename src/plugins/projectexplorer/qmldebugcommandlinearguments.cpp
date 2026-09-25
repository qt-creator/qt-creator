// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldebugcommandlinearguments.h"

#include <projectexplorer/projectexplorerconstants.h>

namespace ProjectExplorer {

QString qmlDebugServices(QmlDebugServicesPreset preset)
{
    switch (preset) {
    case NoQmlDebugServices:
        return QString();
    case QmlDebuggerServices:
        return QStringLiteral("DebugMessages,QmlDebugger,V8Debugger,QmlInspector");
    case QmlProfilerServices:
        return QStringLiteral("CanvasFrameRate,EngineControl,DebugMessages");
    case QmlNativeDebuggerServices:
        return QStringLiteral("NativeQmlDebugger");
    case QmlPreviewServices:
        return QStringLiteral("QmlPreview,DebugTranslation,CanvasFrameRate,EventReplay");
    default:
        Q_ASSERT(false);
        return QString();
    }
}

QString qmlDebugCommandLineArguments(QmlDebugServicesPreset services,
                                     const QString &connectionMode, bool block)
{
    if (services == NoQmlDebugServices)
        return QString();

    return QString::fromLatin1("-qmljsdebugger=%1%2,services:%3").arg(connectionMode)
            .arg(QLatin1String(block ? ",block" : "")).arg(qmlDebugServices(services));
}

static QString tcpArguments(QmlDebugServicesPreset services, const QUrl &server, bool block,
                            bool nameLoopback)
{
    const QString host = server.host();
    const bool bare = host.isEmpty()
                      || (!nameLoopback
                          && (host == QLatin1String("127.0.0.1") || host == QLatin1String("::1")));
    const QString mode = bare ? QString("port:%1").arg(server.port())
                              : QString("host:%1,port:%2").arg(host).arg(server.port());
    return qmlDebugCommandLineArguments(services, mode, block);
}

QString qmlDebugTcpArguments(QmlDebugServicesPreset services, const QUrl &server, bool block)
{
    return tcpArguments(services, server, block, false);
}

QString qmlDebugDesktopTcpArguments(QmlDebugServicesPreset services, const QUrl &server,
                                    bool block)
{
    // Without a host the QML debug server binds all interfaces, so a port found free on the
    // loopback interface alone is no proof that the server can have it.
    return tcpArguments(services, server, block, true);
}

QString qmlDebugNativeArguments(QmlDebugServicesPreset services, bool block)
{
    return qmlDebugCommandLineArguments(services, QLatin1String("native"), block);
}

QString qmlDebugLocalArguments(QmlDebugServicesPreset services, const QString &socket,
                                      bool block)
{
    return qmlDebugCommandLineArguments(services, QLatin1String("file:") + socket, block);
}

Utils::Id runnerIdForRunMode(Utils::Id runMode)
{
    if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        return ProjectExplorer::Constants::QML_PROFILER_RUNNER;
    if (runMode == ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE)
        return ProjectExplorer::Constants::QML_PREVIEW_RUNNER;
    return {};
}

QmlDebugServicesPreset servicesForRunMode(Utils::Id runMode)
{
    if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        return QmlProfilerServices;
    if (runMode == ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE)
        return QmlPreviewServices;
    if (runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE)
        return QmlDebuggerServices;
    return {};
}

} // namespace ProjectExplorer
