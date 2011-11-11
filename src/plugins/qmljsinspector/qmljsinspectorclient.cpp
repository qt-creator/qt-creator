/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljsinspectorclient.h"
#include "qmljsclientproxy.h"
#include "qmljsinspectorconstants.h"

#include <QtGui/QColor>

using namespace QmlJSDebugger;

namespace QmlJSInspector {
namespace Internal {

QmlJSInspectorClient::QmlJSInspectorClient(QDeclarativeDebugConnection *client,
                                         QObject * /*parent*/)
    : QDeclarativeDebugClient(QLatin1String("QDeclarativeObserverMode"), client) ,
    m_connection(client)
{
    setObjectName(name());
}

void QmlJSInspectorClient::statusChanged(Status status)
{
    emit connectedStatusChanged(status);
}

void QmlJSInspectorClient::messageReceived(const QByteArray &message)
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

        if (toolId == Constants::ColorPickerMode) {
            emit colorPickerActivated();
        } else if (toolId == Constants::ZoomMode) {
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

        log(LogReceive, type, paused ? QLatin1String("true") : QLatin1String("false"));

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
    case InspectorProtocol::ColorChanged: {
        QColor col;
        ds >> col;

        log(LogReceive, type, col.name());

        emit selectedColorChanged(col);
        break;
    }
    default:
        qWarning() << "Warning: Not handling message:" << type;
    }
}

QList<int> QmlJSInspectorClient::currentObjects() const
{
    return m_currentDebugIds;
}

void QmlJSInspectorClient::setCurrentObjects(const QList<int> &debugIds)
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

void recurseObjectIdList(const QDeclarativeDebugObjectReference &ref, QList<int> &debugIds, QList<QString> &objectIds)
{
    debugIds << ref.debugId();
    objectIds << ref.idString();
    foreach (const QDeclarativeDebugObjectReference &child, ref.children())
        recurseObjectIdList(child, debugIds, objectIds);
}

void QmlJSInspectorClient::setObjectIdList(const QList<QDeclarativeDebugObjectReference> &objectRoots)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QList<int> debugIds;
    QList<QString> objectIds;

    foreach (const QDeclarativeDebugObjectReference &ref, objectRoots)
        recurseObjectIdList(ref, debugIds, objectIds);

    InspectorProtocol::Message cmd = InspectorProtocol::ObjectIdList;
    ds << cmd
       << debugIds.length();

    Q_ASSERT(debugIds.length() == objectIds.length());

    for(int i = 0; i < debugIds.length(); ++i) {
        ds << debugIds[i] << objectIds[i];
    }

    log(LogSend, cmd, QString("%1 %2 [list of debug / object ids]").arg(debugIds.length()));

    sendMessage(message);
}

void QmlJSInspectorClient::clearComponentCache()
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

void QmlJSInspectorClient::reloadViewer()
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

void QmlJSInspectorClient::setDesignModeBehavior(bool inDesignMode)
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

void QmlJSInspectorClient::setAnimationSpeed(qreal slowDownFactor)
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

void QmlJSInspectorClient::setAnimationPaused(bool paused)
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

void QmlJSInspectorClient::changeToColorPickerTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ChangeTool;
    InspectorProtocol::Tool tool = InspectorProtocol::ColorPickerTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, InspectorProtocol::toString(tool));

    sendMessage(message);
}

void QmlJSInspectorClient::changeToSelectTool()
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

void QmlJSInspectorClient::changeToSelectMarqueeTool()
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

void QmlJSInspectorClient::changeToZoomTool()
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

void QmlJSInspectorClient::showAppOnTop(bool showOnTop)
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

void QmlJSInspectorClient::createQmlObject(const QString &qmlText, int parentDebugId,
                                             const QStringList &imports, const QString &filename, int order)
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

    log(LogSend, cmd, QString("%1 %2 [%3] %4").arg(qmlText, QString::number(parentDebugId),
                                                   imports.join(","), filename));

    sendMessage(message);
}

void QmlJSInspectorClient::destroyQmlObject(int debugId)
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

void QmlJSInspectorClient::reparentQmlObject(int debugId, int newParent)
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


void QmlJSInspectorClient::applyChangesToQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlJSInspectorClient::applyChangesFromQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlJSInspectorClient::log(LogDirection direction, InspectorProtocol::Message message,
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
