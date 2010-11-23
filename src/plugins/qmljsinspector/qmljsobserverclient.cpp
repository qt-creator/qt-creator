/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QColor>

enum {
    debug = true
};

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
        emit animationSpeedChanged(slowdownFactor);
    } else if (type == "SET_DESIGN_MODE") {
        bool inDesignMode;
        ds >> inDesignMode;
        emit designModeBehaviorChanged(inDesignMode);
    } else if (type == "SHOW_APP_ON_TOP") {
        bool showAppOnTop;
        ds >> showAppOnTop;
        emit showAppOnTopChanged(showAppOnTop);
    } else if (type == "RELOADED") {
        emit reloaded();
    } else if (type == "COLOR_CHANGED") {
        QColor col;
        ds >> col;
        emit selectedColorChanged(col);
    } else if (type == "CONTEXT_PATH_UPDATED") {
        QStringList contextPath;
        ds >> contextPath;
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

    ds << QByteArray("SET_CURRENT_OBJECTS")
       << debugIds.length();

    foreach (int id, debugIds) {
        ds << id;
    }

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"SET_CURRENT_OBJECTS" << debugIds.length() << "[list of ids]";

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

    ds << QByteArray("OBJECT_ID_LIST")
       << debugIds.length();

    Q_ASSERT(debugIds.length() == objectIds.length());

    for(int i = 0; i < debugIds.length(); ++i) {
        ds << debugIds[i] << objectIds[i];
    }

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"OBJECT_ID_LIST" << debugIds.length() << "[list of debug / object ids]";

    sendMessage(message);
}

void QmlJSObserverClient::setContextPathIndex(int contextPathIndex)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_CONTEXT_PATH_IDX")
       << contextPathIndex;

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"SET_CONTEXT_PATH_IDX" << contextPathIndex;

    sendMessage(message);
}

void QmlJSObserverClient::clearComponentCache()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CLEAR_COMPONENT_CACHE");

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"CLEAR_COMPONENT_CACHE";

    sendMessage(message);
}

void QmlJSObserverClient::reloadViewer()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("RELOAD");

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"RELOAD";

    sendMessage(message);
}

void QmlJSObserverClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_DESIGN_MODE")
       << inDesignMode;

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"SET_DESIGN_MODE" << inDesignMode;

    sendMessage(message);
}

void QmlJSObserverClient::setAnimationSpeed(qreal slowdownFactor)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_ANIMATION_SPEED")
       << slowdownFactor;

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"SET_ANIMATION_SPEED" << slowdownFactor;

    sendMessage(message);
}

void QmlJSObserverClient::changeToColorPickerTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("COLOR_PICKER");

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"CHANGE_TOOL" << "COLOR_PICKER";

    sendMessage(message);
}

void QmlJSObserverClient::changeToSelectTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("SELECT");

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"CHANGE_TOOL" << "SELECT";

    sendMessage(message);
}

void QmlJSObserverClient::changeToSelectMarqueeTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("SELECT_MARQUEE");

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"CHANGE_TOOL" << "SELECT_MARQUEE";

    sendMessage(message);
}

void QmlJSObserverClient::changeToZoomTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("ZOOM");

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"CHANGE_TOOL" << "ZOOM";

    sendMessage(message);
}

void QmlJSObserverClient::showAppOnTop(bool showOnTop)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SHOW_APP_ON_TOP")
       << showOnTop;

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" <<"SHOWONTOP" << showOnTop;

    sendMessage(message);
}

void QmlJSObserverClient::createQmlObject(const QString &qmlText, int parentDebugId,
                                             const QStringList &imports, const QString &filename)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CREATE_OBJECT")
       << qmlText
       << parentDebugId
       << imports
       << filename;

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" << "CREATE_OBJECT" << qmlText << parentDebugId << imports << filename;

    sendMessage(message);
}

void QmlJSObserverClient::destroyQmlObject(int debugId)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("DESTROY_OBJECT")  << debugId;

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" << "DESTROY_OBJECT" << debugId;

    sendMessage(message);
}

void QmlJSObserverClient::reparentQmlObject(int debugId, int newParent)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("MOVE_OBJECT")
       << debugId
       << newParent;

    if (debug)
        qDebug() << "QmlJSObserverClient: Sending" << "MOVE_OBJECT" << debugId << newParent;

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


} // namespace Internal
} // namespace QmlJSInspector
