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
        return QStringLiteral("DebugMessages,QmlDebugger,V8Debugger,QmlInspector,DebugTranslation");
    case QmlProfilerServices:
        return QStringLiteral("CanvasFrameRate,EngineControl,DebugMessages,DebugTranslation");
    case QmlNativeDebuggerServices:
        return QStringLiteral("NativeQmlDebugger,DebugTranslation");
    case QmlPreviewServices:
        return QStringLiteral("QmlPreview,DebugTranslation");
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

QString qmlDebugTcpArguments(QmlDebugServicesPreset services, const QUrl &server, bool block)
{
    //  TODO: Also generate host:<host> if applicable.
    return qmlDebugCommandLineArguments(services, QString("port:%1").arg(server.port()), block);
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
