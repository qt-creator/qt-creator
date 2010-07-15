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

#include "qmljsdesigndebugclient.h"
#include "qmljsclientproxy.h"
#include "qmljsinspectorconstants.h"

namespace QmlJSInspector {
namespace Internal {

QmlJSDesignDebugClient::QmlJSDesignDebugClient(QDeclarativeDebugConnection *client,
                                                             QObject * /*parent*/)
    : QDeclarativeDebugClient(QLatin1String("QDeclarativeDesignMode"), client) ,
    m_connection(client)
{
    setEnabled(true);
}

void QmlJSDesignDebugClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    QByteArray type;
    ds >> type;

    if (type == "CURRENT_OBJECTS_CHANGED") {
        int objectCount;
        ds >> objectCount;
        m_selectedItemIds.clear();

        for(int i = 0; i < objectCount; ++i) {
            int debugId;
            ds >> debugId;
            if (debugId != -1) {
                m_selectedItemIds  << debugId;
            }
        }

        emit currentObjectsChanged(m_selectedItemIds);
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
    }
}

QList<int> QmlJSDesignDebugClient::selectedItemIds() const
{
    return m_selectedItemIds;
}

void QmlJSDesignDebugClient::setSelectedItemsByObjectId(const QList<QDeclarativeDebugObjectReference> &objects)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_CURRENT_OBJECTS")
       << objects.length();

    foreach(const QDeclarativeDebugObjectReference &ref, objects) {
        ds << ref.debugId();
    }

    sendMessage(message);
}

void QmlJSDesignDebugClient::reloadViewer()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("RELOAD");
    sendMessage(message);
}

void QmlJSDesignDebugClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_DESIGN_MODE")
       << inDesignMode;

    sendMessage(message);
}

void QmlJSDesignDebugClient::setAnimationSpeed(qreal slowdownFactor)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_ANIMATION_SPEED")
       << slowdownFactor;
    sendMessage(message);
}

void QmlJSDesignDebugClient::changeToColorPickerTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("COLOR_PICKER");
    sendMessage(message);
}

void QmlJSDesignDebugClient::changeToSelectTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("SELECT");
    sendMessage(message);
}

void QmlJSDesignDebugClient::changeToSelectMarqueeTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("SELECT_MARQUEE");
    sendMessage(message);
}

void QmlJSDesignDebugClient::changeToZoomTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CHANGE_TOOL")
       << QByteArray("ZOOM");
    sendMessage(message);
}

void QmlJSDesignDebugClient::applyChangesToQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlJSDesignDebugClient::applyChangesFromQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}


} // namespace Internal
} // namespace QmlJSInspector
