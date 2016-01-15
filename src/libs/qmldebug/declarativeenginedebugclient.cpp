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

#include "declarativeenginedebugclient.h"
#include "qmldebugconstants.h"
#include "qmldebugclient.h"
#include "qpacketprotocol.h"

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
    if (state() == Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("SET_BINDING") << objectDebugId << propertyName
           << bindingExpression << isLiteralValue << source << line;
        sendMessage(ds.data());
    }
    return id;
}

quint32 DeclarativeEngineDebugClient::resetBindingForObject(
        int objectDebugId,
        const QString &propertyName)
{
    quint32 id = 0;
    if (state() == Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("RESET_BINDING") << objectDebugId << propertyName;
        sendMessage(ds.data());
    }
    return id;
}

quint32 DeclarativeEngineDebugClient::setMethodBody(
        int objectDebugId, const QString &methodName,
        const QString &methodBody)
{
    quint32 id = 0;
    if (state() == Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(connection()->currentDataStreamVersion());
        ds << QByteArray("SET_METHOD_BODY") << objectDebugId
           << methodName << methodBody;
        sendMessage(ds.data());
    }
    return id;
}

void DeclarativeEngineDebugClient::messageReceived(const QByteArray &data)
{
    QPacket ds(connection()->currentDataStreamVersion(), data);
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
