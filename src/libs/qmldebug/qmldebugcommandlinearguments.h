/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/port.h>
#include <QString>

namespace QmlDebug {

enum QmlDebugServicesPreset {
    NoQmlDebugServices,
    QmlDebuggerServices,
    QmlProfilerServices,
    QmlNativeDebuggerServices
};

static inline QString qmlDebugServices(QmlDebugServicesPreset preset)
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
    default:
        Q_ASSERT(false);
        return QString();
    }
}

static inline QString qmlDebugCommandLineArguments(QmlDebugServicesPreset services,
                                                   const QString &connectionMode, bool block)
{
    if (services == NoQmlDebugServices)
        return QString();

    return QString::fromLatin1("-qmljsdebugger=%1%2,services:%3").arg(connectionMode)
            .arg(QLatin1String(block ? ",block" : "")).arg(qmlDebugServices(services));
}

static inline QString qmlDebugTcpArguments(QmlDebugServicesPreset services,
                                           Utils::Port port = Utils::Port(),
                                           bool block = true)
{
    return qmlDebugCommandLineArguments(services, port.isValid() ?
                                            QString::fromLatin1("port:%1").arg(port.number()) :
                                            QStringLiteral("port:%qml_port%"), block);
}

static inline QString qmlDebugNativeArguments(QmlDebugServicesPreset services, bool block = true)
{
    return qmlDebugCommandLineArguments(services, QLatin1String("native"), block);
}

static inline QString qmlDebugLocalArguments(QmlDebugServicesPreset services, const QString &socket,
                                             bool block = true)
{
    return qmlDebugCommandLineArguments(services, QLatin1String("file:") + socket, block);
}

} // namespace QmlDebug
