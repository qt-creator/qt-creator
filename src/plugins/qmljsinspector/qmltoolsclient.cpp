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

#include "qmltoolsclient.h"
#include "qmljsclientproxy.h"
#include "qmljsinspectorconstants.h"

using namespace QmlJSDebugger;

namespace QmlJSInspector {
namespace Internal {

QmlToolsClient::QmlToolsClient(QmlDebugConnection *client,
                                           QObject * /*parent*/)
    : QmlDebugClient(QLatin1String("QDeclarativeObserverMode"), client),
      m_connection(client)
{
    setObjectName(name());
}

void QmlToolsClient::statusChanged(Status status)
{
    emit connectedStatusChanged(status);
}

void QmlToolsClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    InspectorProtocol::Message type;
    ds >> type;

    switch (type) {
    case InspectorProtocol::CurrentObjectsChanged: {
        int objectCount;
        ds >> objectCount;

        log(LogReceive, type, QString("%1 [list of debug ids]").arg(objectCount));

        m_currentDebugIds.clear();

        for (int i = 0; i < objectCount; ++i) {
            int debugId;
            ds >> debugId;
            if (debugId != -1)
                m_currentDebugIds << debugId;
        }

        emit currentObjectsChanged(m_currentDebugIds);
        break;
    }
    case InspectorProtocol::ToolChanged: {
        int toolId;
        ds >> toolId;

        log(LogReceive, type, QString::number(toolId));

        if (toolId == Constants::ZoomMode) {
            emit zoomToolActivated();
        } else if (toolId == Constants::SelectionToolMode) {
            emit selectToolActivated();
        } else if (toolId == Constants::MarqueeSelectionToolMode) {
            emit selectMarqueeToolActivated();
        }
        break;
    }
    case InspectorProtocol::AnimationSpeedChanged: {
        qreal slowDownFactor;
        ds >> slowDownFactor;

        log(LogReceive, type, QString::number(slowDownFactor));

        emit animationSpeedChanged(slowDownFactor);
        break;
    }
    case InspectorProtocol::AnimationPausedChanged: {
        bool paused;
        ds >> paused;

        log(LogReceive, type, paused ? QLatin1String("true")
                                     : QLatin1String("false"));

        emit animationPausedChanged(paused);
        break;
    }
    case InspectorProtocol::SetDesignMode: {
        bool inDesignMode;
        ds >> inDesignMode;

        log(LogReceive, type, QLatin1String(inDesignMode ? "true" : "false"));

        emit designModeBehaviorChanged(inDesignMode);
        break;
    }
    case InspectorProtocol::ShowAppOnTop: {
        bool showAppOnTop;
        ds >> showAppOnTop;

        log(LogReceive, type, QLatin1String(showAppOnTop ? "true" : "false"));

        emit showAppOnTopChanged(showAppOnTop);
        break;
    }
    case InspectorProtocol::Reloaded: {
        log(LogReceive, type);
        emit reloaded();
        break;
    }
    default:
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

    InspectorProtocol::Message cmd = InspectorProtocol::SetCurrentObjects;
    ds << cmd
       << debugIds.length();

    foreach (int id, debugIds) {
        ds << id;
    }

    log(LogSend, cmd, QString("%1 [list of ids]").arg(debugIds.length()));

    sendMessage(message);
}

void recurseObjectIdList(const QmlDebugObjectReference &ref,
                         QList<int> &debugIds, QList<QString> &objectIds)
{
    debugIds << ref.debugId();
    objectIds << ref.idString();
    foreach (const QmlDebugObjectReference &child, ref.children())
        recurseObjectIdList(child, debugIds, objectIds);
}

void QmlToolsClient::setObjectIdList(
        const QList<QmlDebugObjectReference> &objectRoots)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QList<int> debugIds;
    QList<QString> objectIds;

    foreach (const QmlDebugObjectReference &ref, objectRoots)
        recurseObjectIdList(ref, debugIds, objectIds);

    InspectorProtocol::Message cmd = InspectorProtocol::ObjectIdList;
    ds << cmd
       << debugIds.length();

    Q_ASSERT(debugIds.length() == objectIds.length());

    for(int i = 0; i < debugIds.length(); ++i) {
        ds << debugIds[i] << objectIds[i];
    }

    log(LogSend, cmd,
        QString("%1 %2 [list of debug / object ids]").arg(debugIds.length()));

    sendMessage(message);
}

void QmlToolsClient::clearComponentCache()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ClearComponentCache;
    ds << cmd;

    log(LogSend, cmd);

    sendMessage(message);
}

void QmlToolsClient::reloadViewer()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::Reload;
    ds << cmd;

    log(LogSend, cmd);

    sendMessage(message);
}

void QmlToolsClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::SetDesignMode;
    ds << cmd
       << inDesignMode;

    log(LogSend, cmd, QLatin1String(inDesignMode ? "true" : "false"));

    sendMessage(message);
}

void QmlToolsClient::setAnimationSpeed(qreal slowDownFactor)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::SetAnimationSpeed;
    ds << cmd
       << slowDownFactor;


    log(LogSend, cmd, QString::number(slowDownFactor));

    sendMessage(message);
}

void QmlToolsClient::setAnimationPaused(bool paused)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::SetAnimationPaused;
    ds << cmd
       << paused;

    log(LogSend, cmd, paused ? QLatin1String("true") : QLatin1String("false"));

    sendMessage(message);
}

void QmlToolsClient::changeToSelectTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ChangeTool;
    InspectorProtocol::Tool tool = InspectorProtocol::SelectTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, InspectorProtocol::toString(tool));

    sendMessage(message);
}

void QmlToolsClient::changeToSelectMarqueeTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ChangeTool;
    InspectorProtocol::Tool tool = InspectorProtocol::SelectMarqueeTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, InspectorProtocol::toString(tool));

    sendMessage(message);
}

void QmlToolsClient::changeToZoomTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ChangeTool;
    InspectorProtocol::Tool tool = InspectorProtocol::ZoomTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, InspectorProtocol::toString(tool));

    sendMessage(message);
}

void QmlToolsClient::showAppOnTop(bool showOnTop)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ShowAppOnTop;
    ds << cmd << showOnTop;

    log(LogSend, cmd, QLatin1String(showOnTop ? "true" : "false"));

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

    InspectorProtocol::Message cmd = InspectorProtocol::CreateObject;
    ds << cmd
       << qmlText
       << parentDebugId
       << imports
       << filename
       << order;

    log(LogSend, cmd, QString("%1 %2 [%3] %4").arg(qmlText,
                                                   QString::number(parentDebugId),
                                                   imports.join(","), filename));

    sendMessage(message);
}

void QmlToolsClient::destroyQmlObject(int debugId)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::DestroyObject;
    ds << cmd << debugId;

    log(LogSend, cmd, QString::number(debugId));

    sendMessage(message);
}

void QmlToolsClient::reparentQmlObject(int debugId, int newParent)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::MoveObject;
    ds << cmd
       << debugId
       << newParent;

    log(LogSend, cmd, QString("%1 %2").arg(QString::number(debugId),
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
                               InspectorProtocol::Message message,
                               const QString &extra)
{
    QString msg;
    if (direction == LogSend)
        msg += QLatin1String(" sending ");
    else
        msg += QLatin1String(" receiving ");

    msg += InspectorProtocol::toString(message);
    msg += QLatin1Char(' ');
    msg += extra;
    emit logActivity(name(), msg);
}

} // namespace Internal
} // namespace QmlJSInspector
