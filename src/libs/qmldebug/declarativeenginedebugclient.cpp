/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "declarativeenginedebugclient.h"
#include "qmldebugconstants.h"

namespace QmlDebug {

DeclarativeEngineDebugClient::DeclarativeEngineDebugClient(
        QmlDebugConnection *connection)
    : BaseEngineDebugClient(QLatin1String(Constants::QDECLARATIVE_ENGINE), connection)
{
}

quint32 DeclarativeEngineDebugClient::setBindingForObject(
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
        ds << QByteArray("SET_BINDING") << objectDebugId << propertyName
           << bindingExpression << isLiteralValue << source << line;
        sendMessage(message);
    }
    return id;
}

quint32 DeclarativeEngineDebugClient::resetBindingForObject(
        int objectDebugId,
        const QString &propertyName)
{
    quint32 id = 0;
    if (status() == Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("RESET_BINDING") << objectDebugId << propertyName;
        sendMessage(message);
    }
    return id;
}

quint32 DeclarativeEngineDebugClient::setMethodBody(
        int objectDebugId, const QString &methodName,
        const QString &methodBody)
{
    quint32 id = 0;
    if (status() == Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_METHOD_BODY") << objectDebugId
           << methodName << methodBody;
        sendMessage(message);
    }
    return id;
}

void DeclarativeEngineDebugClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray type;
    ds >> type;

    if (type == "OBJECT_CREATED") {
        int engineId;
        int objectId;
        ds >> engineId >> objectId;
        emit newObject(engineId, objectId, -1);
        return;
    } else {
        BaseEngineDebugClient::messageReceived(data);
    }
}
} // namespace QmlDebug
