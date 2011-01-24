/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmljsobserverclient.h"
#include "qmljsclientproxy.h"
#include "qmljsinspectorconstants.h"

#include <QtGui/QColor>

namespace QmlJSInspector {
namespace Internal {

QmlJSObserverClient::QmlJSObserverClient(QDeclarativeDebugConnection *client,
                                                             QObject * /*parent*/)
    : QDeclarativeDebugClient(QLatin1String("QDeclarativeObserverMode"), client) ,
    m_connection(client)
{
}

void QmlJSObserverClient::statusChanged(Status status)
{
    emit connectedStatusChanged(status);
}

void QmlJSObserverClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    QByteArray type;
    ds >> type;

    if (type == "CURRENT_OBJECTS_CHANGED") {
        int objectCount;
        ds >> objectCount;

        log(LogReceive, QString("%1 %2 [list of debug ids]").arg(QString(type),
                                                                 QString::number(objectCount)));

        m_currentDebugIds.clear();

        for(int i = 0; i < objectCount; ++i) {
            int debugId;
            ds >> debugId;
            if (debugId != -1) {
                m_currentDebugIds << debugId;
            }
        }

        emit currentObjectsChanged(m_currentDebugIds);
    } else if (type == "TOOL_CHANGED") {
        int toolId;
        ds >> toolId;

        log(LogReceive, QString("%1 %2").arg(QString(type), QString::number(toolId)));

        if (toolId == Constants::ColorPickerMode) {
            emit colorPickerActivated();
        } else if (toolId == Constants::ZoomMode) {
            emit zoomToolActivated();
        } else if (toolId == Constants::SelectionToolMode) {
            emit selectToolActivated();
        } else if (toolId == Constants::MarqueeSelectionToolMode) {
            emit selectMarqueeToolActivated();
        }
    } else if (type == "ANIMATION_SPEED_CHANGED") {
        qreal slowdownFactor;
        ds >> slowdownFactor;

        log(LogReceive, QString("%1 %2").arg(QString(type), QString::number(slowdownFactor)));

        emit animationSpeedChanged(slowdownFactor);
    } else if (type == "SET_DESIGN_MODE") {
        bool inDesignMode;
        ds >> inDesignMode;

        log(LogReceive, QString("%1 %2").arg(QString(type), inDesignMode ? "true" : "false"));

        emit designModeBehaviorChanged(inDesignMode);
    } else if (type == "SHOW_APP_ON_TOP") {
        bool showAppOnTop;
        ds >> showAppOnTop;

        log(LogReceive, QString("%1 %2").arg(QString(type), showAppOnTop ? "true" : "false"));

        emit showAppOnTopChanged(showAppOnTop);
    } else if (type == "RELOADED") {

        log(LogReceive, type);

        emit reloaded();
    } else if (type == "COLOR_CHANGED") {
        QColor col;
        ds >> col;

        log(LogReceive, QString("%1 %2").arg(QString(type), col.name()));

        emit selectedColorChanged(col);
    } else if (type == "CONTEXT_PATH_UPDATED") {
        QStringList contextPath;
        ds >> contextPath;

        log(LogReceive, QString("%1 %2").arg(QString(type), contextPath.join(", ")));

        emit contextPathUpdated(contextPath);
    }
}

QList<int> QmlJSObserverClient::currentObjects() const
{
    return m_currentDebugIds;
}

void QmlJSObserverClient::setCurrentObjects(const QList<int> &debugIds) {
    if (!m_connection || !m_connection->isConnected())
        return;

    if (debugIds == m_currentDebugIds)
        return;

    m_currentDebugIds = debugIds;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "SET_CURRENT_OBJECTS";
    ds << cmd
       << debugIds.length();

    foreach (int id, debugIds) {
        ds << id;
    }

    log(LogSend, QString("%1 %2 [list of ids]").arg(QString(cmd),
                                                    QString::number(debugIds.length())));

    sendMessage(message);
}

void recurseObjectIdList(const QDeclarativeDebugObjectReference &ref, QList<int> &debugIds, QList<QString> &objectIds)
{
    debugIds << ref.debugId();
    objectIds << ref.idString();
    foreach(const QDeclarativeDebugObjectReference &child, ref.children()) {
        recurseObjectIdList(child, debugIds, objectIds);
    }
}

void QmlJSObserverClient::setObjectIdList(const QList<QDeclarativeDebugObjectReference> &objectRoots)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QList<int> debugIds;
    QList<QString> objectIds;

    foreach(const QDeclarativeDebugObjectReference &ref, objectRoots) {
        recurseObjectIdList(ref, debugIds, objectIds);
    }

    QByteArray cmd = "OBJECT_ID_LIST";
    ds << cmd
       << debugIds.length();

    Q_ASSERT(debugIds.length() == objectIds.length());

    for(int i = 0; i < debugIds.length(); ++i) {
        ds << debugIds[i] << objectIds[i];
    }

    log(LogSend, QString("%1 %2 [list of debug / object ids]").arg(QString(cmd),
                                                                   QString::number(debugIds.length())));

    sendMessage(message);
}

void QmlJSObserverClient::setContextPathIndex(int contextPathIndex)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "SET_CONTEXT_PATH_IDX";
    ds << cmd
       << contextPathIndex;

    log(LogSend, QString("%1 %2").arg(QString(cmd), contextPathIndex));

    sendMessage(message);
}

void QmlJSObserverClient::clearComponentCache()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "CLEAR_COMPONENT_CACHE";
    ds << cmd;

    log(LogSend, cmd);

    sendMessage(message);
}

void QmlJSObserverClient::reloadViewer()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "RELOAD";
    ds << cmd;

    log(LogSend, cmd);

    sendMessage(message);
}

void QmlJSObserverClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "SET_DESIGN_MODE";
    ds << cmd
       << inDesignMode;

    log(LogSend, QString("%1 %2").arg(QString(cmd), inDesignMode ? "true" : "false"));

    sendMessage(message);
}

void QmlJSObserverClient::setAnimationSpeed(qreal slowdownFactor)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "SET_ANIMATION_SPEED";
    ds << cmd
       << slowdownFactor;


    log(LogSend, QString("%1 %2").arg(QString(cmd),  QString::number(slowdownFactor)));

    sendMessage(message);
}

void QmlJSObserverClient::changeToColorPickerTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "CHANGE_TOOL";
    QByteArray tool = "COLOR_PICKER";
    ds << cmd
       << tool;

    log(LogSend, QString("%1 %2").arg(QString(cmd), QString(tool)));

    sendMessage(message);
}

void QmlJSObserverClient::changeToSelectTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "CHANGE_TOOL";
    QByteArray tool = "SELECT";
    ds << cmd
       << tool;

    log(LogSend, QString("%1 %2").arg(QString(cmd), QString(tool)));

    sendMessage(message);
}

void QmlJSObserverClient::changeToSelectMarqueeTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "CHANGE_TOOL";
    QByteArray tool = "SELECT_MARQUEE";
    ds << cmd
       << tool;

    log(LogSend, QString("%1 %2").arg(QString(cmd), QString(tool)));

    sendMessage(message);
}

void QmlJSObserverClient::changeToZoomTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "CHANGE_TOOL";
    QByteArray tool = "ZOOM";
    ds << cmd
       << tool;

    log(LogSend, QString("%1 %2").arg(QString(cmd), QString(tool)));

    sendMessage(message);
}

void QmlJSObserverClient::showAppOnTop(bool showOnTop)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "SHOW_APP_ON_TOP";
    ds << cmd << showOnTop;

    log(LogSend, QString("%1 %2").arg(QString(cmd), showOnTop ? "true" : "false"));

    sendMessage(message);
}

void QmlJSObserverClient::createQmlObject(const QString &qmlText, int parentDebugId,
                                             const QStringList &imports, const QString &filename)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "CREATE_OBJECT";
    ds << cmd
       << qmlText
       << parentDebugId
       << imports
       << filename;

    log(LogSend, QString("%1 %2 %3 [%4] %5").arg(QString(cmd), qmlText, QString::number(parentDebugId),
                                               imports.join(","), filename));

    sendMessage(message);
}

void QmlJSObserverClient::destroyQmlObject(int debugId)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "DESTROY_OBJECT";
    ds << cmd << debugId;

    log(LogSend, QString("%1 %2").arg(QString(cmd), debugId));

    sendMessage(message);
}

void QmlJSObserverClient::reparentQmlObject(int debugId, int newParent)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QByteArray cmd = "MOVE_OBJECT";
    ds << cmd
       << debugId
       << newParent;

    log(LogSend, QString("%1 %2 %3").arg(QString(cmd), QString::number(debugId),
                                         QString::number(newParent)));

    sendMessage(message);
}


void QmlJSObserverClient::applyChangesToQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlJSObserverClient::applyChangesFromQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlJSObserverClient::log(LogDirection direction, const QString &message)
{
    QString msg;
    if (direction == LogSend) {
        msg += " sending ";
    } else {
        msg += " receiving ";
    }
    msg += message;
    emit logActivity(name(), msg);
}

} // namespace Internal
} // namespace QmlJSInspector
