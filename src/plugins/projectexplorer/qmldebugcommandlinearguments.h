// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>

#include <QString>
#include <QUrl>

namespace ProjectExplorer {

enum QmlDebugServicesPreset {
    NoQmlDebugServices,
    QmlDebuggerServices,
    QmlProfilerServices,
    QmlNativeDebuggerServices,
    QmlPreviewServices
};

PROJECTEXPLORER_EXPORT QString qmlDebugServices(QmlDebugServicesPreset preset);

PROJECTEXPLORER_EXPORT QString qmlDebugCommandLineArguments(QmlDebugServicesPreset services,
                                                            const QString &connectionMode,
                                                            bool block);

PROJECTEXPLORER_EXPORT QString qmlDebugTcpArguments(QmlDebugServicesPreset services,
                                                    const QUrl &server,
                                                    bool block = true);

PROJECTEXPLORER_EXPORT QString qmlDebugNativeArguments(QmlDebugServicesPreset services,
                                                       bool block = true);

PROJECTEXPLORER_EXPORT QString qmlDebugLocalArguments(QmlDebugServicesPreset services,
                                                      const QString &socket,
                                                      bool block = true);

PROJECTEXPLORER_EXPORT Utils::Id runnerIdForRunMode(Utils::Id runMode);

PROJECTEXPLORER_EXPORT QmlDebugServicesPreset servicesForRunMode(Utils::Id runMode);

} // namespace ProjectExplorer
