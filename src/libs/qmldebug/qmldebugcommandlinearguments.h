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
        return QStringLiteral("CanvasFrameRate,EngineControl");
    case QmlNativeDebuggerServices:
        return QStringLiteral("NativeQmlDebugger");
    default:
        Q_ASSERT(false);
        return QString();
    }
}

static inline QString qmlDebugCommandLineArguments(QmlDebugServicesPreset services,
                                                   quint16 port = 0)
{
    if (services == NoQmlDebugServices)
        return QString();

    if (services == QmlNativeDebuggerServices)
        return QString::fromLatin1("-qmljsdebugger=native,services:%1")
            .arg(qmlDebugServices(services));

    return QString::fromLatin1("-qmljsdebugger=port:%1,block,services:%2")
            .arg(port ? QString::number(port) : QStringLiteral("%qml_port%"))
            .arg(qmlDebugServices(services));
}

} // namespace QmlDebug

#endif // QMLDEBUGCOMMANDLINEARGUMENTS_H

