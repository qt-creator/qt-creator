/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlenginedebugclient.h"
#include "qmldebugconstants.h"

namespace QmlDebug {

QmlEngineDebugClient::QmlEngineDebugClient(
        QmlDebugConnection *connection)
    : BaseEngineDebugClient(QLatin1String(Constants::QML_DEBUGGER), connection)
{
}

quint32 QmlEngineDebugClient::setBindingForObject(
        int objectDebugId,
        const QString &propertyName,
        const QVariant &bindingExpression,
        bool isLiteralValue,
        QString source, int line)
{
    quint32 id = 0;
    if (status() == Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_BINDING") << id << objectDebugId << propertyName
           << bindingExpression << isLiteralValue << source << line;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::resetBindingForObject(
        int objectDebugId,
        const QString &propertyName)
{
    quint32 id = 0;
    if (status() == Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("RESET_BINDING") << id << objectDebugId << propertyName;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::setMethodBody(
        int objectDebugId, const QString &methodName,
        const QString &methodBody)
{
    quint32 id = 0;
    if (status() == Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_METHOD_BODY") << id << objectDebugId
           << methodName << methodBody;
        sendMessage(message);
    }
    return id;
}

void QmlEngineDebugClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    int queryId;
    QByteArray type;
    ds >> type >> queryId;

    if (type == "OBJECT_CREATED") {
        int engineId;
        int objectId;
        int parentId;
        ds >> engineId >> objectId >> parentId;
        emit newObject(engineId, objectId, parentId);
        return;
    } else {
        BaseEngineDebugClient::messageReceived(data);
    }
}
} // namespace QmlDebug
