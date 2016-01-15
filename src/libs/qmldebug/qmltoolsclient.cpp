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

#include "qmltoolsclient.h"
#include <qmldebug/qpacketprotocol.h>
#include <QStringList>

//INSPECTOR SERVICE PROTOCOL
// <HEADER><COMMAND><DATA>
// <HEADER> : <type{request, response, event}><requestId/eventId>[<response_success_bool>]
// <COMMAND> : {"enable", "disable", "reload", "showAppOnTop"}
// <DATA> : select: <debugIds_int_list>
//          reload: <hash<changed_filename_string, filecontents_bytearray>>
//          showAppOnTop: <set_bool>

const char REQUEST[] = "request";
const char RESPONSE[] = "response";
const char EVENT[] = "event";
const char ENABLE[] = "enable";
const char DISABLE[] = "disable";
const char SELECT[] = "select";
const char SHOW_APP_ON_TOP[] = "showAppOnTop";

namespace QmlDebug {

QmlToolsClient::QmlToolsClient(QmlDebugConnection *client)
    : BaseToolsClient(client, QLatin1String("QmlInspector")),
      m_connection(client),
      m_requestId(0)
{
    setObjectName(name());
}

void QmlToolsClient::messageReceived(const QByteArray &message)
{
    QPacket ds(connection()->currentDataStreamVersion(), message);

    QByteArray type;
    int requestId;
    ds >> type >> requestId;

    if (type == QByteArray(RESPONSE)) {
        bool success = false;
        ds >> success;

        log(LogReceive, type, QString::fromLatin1("requestId: %1 success: %2")
            .arg(QString::number(requestId)).arg(QString::number(success)));
    } else if (type == QByteArray(EVENT)) {
        QByteArray event;
        ds >> event;
        if (event == QByteArray(SELECT)) {
            QList<int> debugIds;
            ds >> debugIds;

            debugIds.removeAll(-1);

            QStringList debugIdStrings;
            foreach (int debugId, debugIds) {
                debugIdStrings << QString::number(debugId);
            }
            log(LogReceive, type + ':' + event,
                QString::fromLatin1("[%1]").arg(debugIdStrings.join(QLatin1Char(','))));
            emit currentObjectsChanged(debugIds);
        }
    } else {
        log(LogReceive, type, QLatin1String("Warning: Not handling message"));
    }
}

void QmlToolsClient::setObjectIdList(const QList<ObjectReference> &objectRoots)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QList<int> debugIds;
    foreach (const ObjectReference &object, objectRoots)
        debugIds << object.debugId();

    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray(REQUEST) << m_requestId++ << QByteArray(SELECT) << debugIds;
    sendMessage(ds.data());
}

void QmlToolsClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray(REQUEST) << m_requestId++;
    if (inDesignMode)
        ds << QByteArray(ENABLE);
    else
        ds << QByteArray(DISABLE);

    log(LogSend, ENABLE, QLatin1String(inDesignMode ? "true" : "false"));

    sendMessage(ds.data());
}

void QmlToolsClient::changeToSelectTool()
{
// NOT IMPLEMENTED
}

void QmlToolsClient::changeToSelectMarqueeTool()
{
// NOT IMPLEMENTED
}

void QmlToolsClient::changeToZoomTool()
{
// NOT IMPLEMENTED
}

void QmlToolsClient::showAppOnTop(bool showOnTop)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(SHOW_APP_ON_TOP) << showOnTop;

    log(LogSend, SHOW_APP_ON_TOP, QLatin1String(showOnTop ? "true" : "false"));

    sendMessage(ds.data());
}

void QmlToolsClient::log(LogDirection direction,
                               const QByteArray &message,
                               const QString &extra)
{
    QString msg;
    if (direction == LogSend)
        msg += QLatin1String("sending ");
    else
        msg += QLatin1String("receiving ");

    msg += QLatin1String(message);
    msg += QLatin1Char(' ');
    msg += extra;
    emit logActivity(name(), msg);
}

} // namespace QmlDebug
