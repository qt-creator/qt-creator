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

#include "qmltoolsclient.h"
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
const char RELOAD[] = "reload";
const char SHOW_APP_ON_TOP[] = "showAppOnTop";

namespace QmlDebug {

QmlToolsClient::QmlToolsClient(QmlDebugConnection *client)
    : BaseToolsClient(client, QLatin1String("QmlInspector")),
      m_connection(client),
      m_requestId(0),
      m_reloadQueryId(-1)
{
    setObjectName(name());
}

void QmlToolsClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    QByteArray type;
    int requestId;
    ds >> type >> requestId;

    if (type == QByteArray(RESPONSE)) {
        bool success = false;
        ds >> success;

        if ((m_reloadQueryId != -1) && (m_reloadQueryId == requestId) && success)
            emit reloaded();

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

void QmlToolsClient::setObjectIdList(
        const QList<ObjectReference> &/*objectRoots*/)
{
    //NOT IMPLEMENTED
}

void QmlToolsClient::reload(const QHash<QString, QByteArray> &changesHash)
{
    if (!m_connection || !m_connection->isOpen())
        return;

    m_reloadQueryId = m_requestId;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(RELOAD) << changesHash;

    log(LogSend, RELOAD);

    sendMessage(message);
}

void QmlToolsClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isOpen())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++;
    if (inDesignMode)
        ds << QByteArray(ENABLE);
    else
        ds << QByteArray(DISABLE);

    log(LogSend, ENABLE, QLatin1String(inDesignMode ? "true" : "false"));

    sendMessage(message);
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
    if (!m_connection || !m_connection->isOpen())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(SHOW_APP_ON_TOP) << showOnTop;

    log(LogSend, SHOW_APP_ON_TOP, QLatin1String(showOnTop ? "true" : "false"));

    sendMessage(message);
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
