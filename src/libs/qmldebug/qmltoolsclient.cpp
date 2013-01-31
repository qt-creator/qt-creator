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

#include "qmltoolsclient.h"
#include <QStringList>

//INSPECTOR SERVICE PROTOCOL
// <HEADER><COMMAND><DATA>
// <HEADER> : <type{request, response, event}><requestId/eventId>[<response_success_bool>]
// <COMMAND> : {"enable", "disable", "select", "reload", "setAnimationSpeed",
//              "showAppOnTop", "createObject", "destroyObject", "moveObject",
//              "clearCache"}
// <DATA> : select: <debugIds_int_list>
//          reload: <hash<changed_filename_string, filecontents_bytearray>>
//          setAnimationSpeed: <speed_real>
//          showAppOnTop: <set_bool>
//          createObject: <qml_string><parentId_int><imports_string_list><filename_string>
//          destroyObject: <debugId_int>
//          moveObject: <debugId_int><newParentId_int>
//          clearCache: void

const char REQUEST[] = "request";
const char RESPONSE[] = "response";
const char EVENT[] = "event";
const char ENABLE[] = "enable";
const char DISABLE[] = "disable";
const char SELECT[] = "select";
const char RELOAD[] = "reload";
const char SET_ANIMATION_SPEED[] = "setAnimationSpeed";
const char SHOW_APP_ON_TOP[] = "showAppOnTop";
const char CREATE_OBJECT[] = "createObject";
const char DESTROY_OBJECT[] = "destroyObject";
const char MOVE_OBJECT[] = "moveObject";
const char CLEAR_CACHE[] = "clearCache";

namespace QmlDebug {

QmlToolsClient::QmlToolsClient(QmlDebugConnection *client)
    : BaseToolsClient(client, QLatin1String("QmlInspector")),
      m_connection(client),
      m_requestId(0),
      m_slowDownFactor(1),
      m_reloadQueryId(-1),
      m_destroyObjectQueryId(-1)
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

        if ((m_destroyObjectQueryId != -1) && (m_destroyObjectQueryId == requestId)
                && success && !ds.atEnd()) {
            int objectDebugId;
            ds >> objectDebugId;
            emit destroyedObject(objectDebugId);
        }

        log(LogReceive, type, QString(QLatin1String("requestId: %1 success: %2"))
            .arg(QString::number(requestId)).arg(QString::number(success)));
    } else if (type == QByteArray(EVENT)) {
        QByteArray event;
        ds >> event;
        if (event == QByteArray(SELECT)) {
            m_currentDebugIds.clear();
            QList<int> debugIds;
            ds >> debugIds;

            QStringList debugIdStrings;
            foreach (int debugId, debugIds) {
                if (debugId != -1)  {
                    m_currentDebugIds << debugId;
                    debugIdStrings << QString::number(debugId);
                }
            }
            log(LogReceive, type + ':' + event,
                QString::fromLatin1("[%1]").arg(debugIdStrings.join(QLatin1String(","))));
            emit currentObjectsChanged(m_currentDebugIds);
        }
    } else {
        log(LogReceive, type, QLatin1String("Warning: Not handling message"));
    }
}

QList<int> QmlToolsClient::currentObjects() const
{
    return m_currentDebugIds;
}

void QmlToolsClient::setCurrentObjects(const QList<int> &debugIds)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    if (debugIds == m_currentDebugIds)
        return;

    m_currentDebugIds = debugIds;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(SELECT) << m_currentDebugIds;

    log(LogSend, SELECT, QString::fromLatin1("%1 [list of ids]").arg(debugIds.length()));

    sendMessage(message);
}

void QmlToolsClient::setObjectIdList(
        const QList<ObjectReference> &/*objectRoots*/)
{
    //NOT IMPLEMENTED
}

void QmlToolsClient::clearComponentCache()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(CLEAR_CACHE);

    log(LogSend, CLEAR_CACHE);

    sendMessage(message);
}

void QmlToolsClient::reload(const QHash<QString, QByteArray> &changesHash)
{
    if (!m_connection || !m_connection->isConnected())
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
    if (!m_connection || !m_connection->isConnected())
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

void QmlToolsClient::setAnimationSpeed(qreal slowDownFactor)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(SET_ANIMATION_SPEED) << slowDownFactor;

    log(LogSend, SET_ANIMATION_SPEED, QString::number(slowDownFactor));

    sendMessage(message);
    //Cache non-zero values
    if (slowDownFactor)
        m_slowDownFactor = slowDownFactor;
}

void QmlToolsClient::setAnimationPaused(bool paused)
{
    if (paused)
        setAnimationSpeed(0);
    else
        setAnimationSpeed(m_slowDownFactor);
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

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(SHOW_APP_ON_TOP) << showOnTop;

    log(LogSend, SHOW_APP_ON_TOP, QLatin1String(showOnTop ? "true" : "false"));

    sendMessage(message);
}

void QmlToolsClient::createQmlObject(const QString &qmlText,
                                           int parentDebugId,
                                           const QStringList &imports,
                                           const QString &filename, int order)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(CREATE_OBJECT)
       << qmlText
       << parentDebugId
       << imports
       << filename
       << order;

    log(LogSend, CREATE_OBJECT, QString::fromLatin1("%1 %2 [%3] %4").arg(qmlText,
                                                   QString::number(parentDebugId),
                                                   imports.join(QLatin1String(",")), filename));

    sendMessage(message);
}

void QmlToolsClient::destroyQmlObject(int debugId)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    m_destroyObjectQueryId = m_requestId;
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(DESTROY_OBJECT) << debugId;

    log(LogSend, DESTROY_OBJECT, QString::number(debugId));

    sendMessage(message);
}

void QmlToolsClient::reparentQmlObject(int debugId, int newParent)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray(REQUEST) << m_requestId++
       << QByteArray(MOVE_OBJECT) << debugId << newParent;

    log(LogSend, MOVE_OBJECT, QString::fromLatin1("%1 %2").arg(QString::number(debugId),
                                           QString::number(newParent)));

    sendMessage(message);
}


void QmlToolsClient::applyChangesToQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlToolsClient::applyChangesFromQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
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
