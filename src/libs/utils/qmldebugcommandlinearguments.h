// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>
#include <QUrl>

namespace Utils {

enum QmlDebugServicesPreset {
    NoQmlDebugServices,
    QmlDebuggerServices,
    QmlProfilerServices,
    QmlNativeDebuggerServices,
    QmlPreviewServices
};

QTCREATOR_UTILS_EXPORT QString qmlDebugServices(QmlDebugServicesPreset preset);

QTCREATOR_UTILS_EXPORT QString qmlDebugCommandLineArguments(QmlDebugServicesPreset services,
                                                            const QString &connectionMode,
                                                            bool block);

QTCREATOR_UTILS_EXPORT QString qmlDebugTcpArguments(QmlDebugServicesPreset services,
                                                    const QUrl &server,
                                                    bool block = true);

QTCREATOR_UTILS_EXPORT QString qmlDebugDesktopTcpArguments(QmlDebugServicesPreset services,
                                                           const QUrl &server,
                                                           bool block = true);

QTCREATOR_UTILS_EXPORT QString qmlDebugNativeArguments(QmlDebugServicesPreset services,
                                                       bool block = true);

QTCREATOR_UTILS_EXPORT QString qmlDebugLocalArguments(QmlDebugServicesPreset services,
                                                      const QString &socket,
                                                      bool block = true);

} // namespace Utils
