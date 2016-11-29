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

#include "qdebugmessageclient.h"

#include <QDataStream>

namespace QmlDebug {

QDebugMessageClient::QDebugMessageClient(QmlDebugConnection *client)
    : QmlDebugClient(QLatin1String("DebugMessages"), client)
{
}

void QDebugMessageClient::stateChanged(State state)
{
    emit newState(state);
}

void QDebugMessageClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == "MESSAGE") {
        int type;
        int line;
        QByteArray debugMessage;
        QByteArray file;
        QByteArray function;
        ds >> type >> debugMessage >> file >> line >> function;
        QDebugContextInfo info;
        info.line = line;
        info.file = QString::fromUtf8(file);
        info.function = QString::fromUtf8(function);
        info.timestamp = -1;
        if (!ds.atEnd()) {
            QByteArray category;
            ds >> category;
            info.category = QString::fromUtf8(category);
            if (!ds.atEnd())
                ds >> info.timestamp;
        }
        emit message(QtMsgType(type), QString::fromUtf8(debugMessage), info);
    }
}

} // namespace QmlDebug
