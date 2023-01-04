// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
