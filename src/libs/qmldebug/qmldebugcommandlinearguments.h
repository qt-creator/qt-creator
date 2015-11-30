/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLDEBUGCOMMANDLINEARGUMENTS_H
#define QMLDEBUGCOMMANDLINEARGUMENTS_H

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

static inline QString qmlDebugTcpArguments(QmlDebugServicesPreset services, quint16 port = 0,
                                           bool block = true)
{
    return qmlDebugCommandLineArguments(services, port ? QString::fromLatin1("port:%1").arg(port) :
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

#endif // QMLDEBUGCOMMANDLINEARGUMENTS_H

