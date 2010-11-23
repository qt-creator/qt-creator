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

#include "qdeclarativeobserverservice.h"

#include <QStringList>
#include <QColor>

#include <QDebug>

namespace QmlJSDebugger {

Q_GLOBAL_STATIC(QDeclarativeObserverService, serviceInstance)

QDeclarativeObserverService::QDeclarativeObserverService()
    : QDeclarativeDebugService(QLatin1String("QDeclarativeObserverMode"))
{
}

QDeclarativeObserverService *QDeclarativeObserverService::instance()
{
    return serviceInstance();
}

void QDeclarativeObserverService::statusChanged(Status status)
{
    emit debuggingClientChanged((status == Enabled));
}

void QDeclarativeObserverService::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    QByteArray type;
    ds >> type;

    if (type == "SET_CURRENT_OBJECTS") {
        int itemCount = 0;
        ds >> itemCount;

        QList<QObject*> selectedObjects;
        for(int i = 0; i < itemCount; ++i) {
            int debugId = -1;
            ds >> debugId;
            QObject *obj = objectForId(debugId);

            if (obj)
                selectedObjects << obj;
        }

        emit currentObjectsChanged(selectedObjects);

    } else if (type == "RELOAD") {
        emit reloadRequested();
    } else if (type == "SET_ANIMATION_SPEED") {
        qreal speed;
        ds >> speed;
        emit animationSpeedChangeRequested(speed);
    } else if (type == "CHANGE_TOOL") {
        QByteArray toolName;
        ds >> toolName;
        if (toolName == "COLOR_PICKER") {
            emit colorPickerToolRequested();
        } else if (toolName == "SELECT") {
            emit selectToolRequested();
        } else if (toolName == "SELECT_MARQUEE") {
            emit selectMarqueeToolRequested();
        } else if (toolName == "ZOOM") {
            emit zoomToolRequested();
        }
    } else if (type == "SET_DESIGN_MODE") {
        bool inDesignMode;
        ds >> inDesignMode;
        emit designModeBehaviorChanged(inDesignMode);
    } else if (type == "SHOW_APP_ON_TOP") {
        bool showOnTop;
        ds >> showOnTop;
        emit showAppOnTopChanged(showOnTop);
    } else if (type == "CREATE_OBJECT") {
        QString qml;
        int parentId;
        QString filename;
        QStringList imports;
        ds >> qml >> parentId >> imports >> filename;
        emit objectCreationRequested(qml, objectForId(parentId), imports, filename);
    } else if (type == "DESTROY_OBJECT") {
        int debugId;
        ds >> debugId;
        if (QObject* obj = objectForId(debugId))
            obj->deleteLater();
    } else if (type == "MOVE_OBJECT") {
       int debugId, newParent;
       ds >> debugId >> newParent;
       emit objectReparentRequested(objectForId(debugId), objectForId(newParent));
    } else if (type == "OBJECT_ID_LIST") {
        int itemCount;
        ds >> itemCount;
        m_stringIdForObjectId.clear();
        for(int i = 0; i < itemCount; ++i) {
            int itemDebugId;
            QString itemIdString;
            ds >> itemDebugId
               >> itemIdString;

            m_stringIdForObjectId.insert(itemDebugId, itemIdString);
        }
    } else if (type == "SET_CONTEXT_PATH_IDX") {
        int contextPathIndex;
        ds >> contextPathIndex;
        emit contextPathIndexChanged(contextPathIndex);
    } else if (type == "CLEAR_COMPONENT_CACHE") {
        emit clearComponentCacheRequested();
    }
}

void QDeclarativeObserverService::setDesignModeBehavior(bool inDesignMode)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_DESIGN_MODE")
       << inDesignMode;

    sendMessage(message);
}

void QDeclarativeObserverService::setCurrentObjects(QList<QObject*> objects)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CURRENT_OBJECTS_CHANGED")
       << objects.length();

    foreach(QObject *object, objects) {
        int id = idForObject(object);
        ds << id;
    }

    sendMessage(message);
}

void QDeclarativeObserverService::setCurrentTool(QmlJSDebugger::Constants::DesignTool toolId)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("TOOL_CHANGED")
       << toolId;

    sendMessage(message);
}

void QDeclarativeObserverService::setAnimationSpeed(qreal slowdownFactor)
{

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("ANIMATION_SPEED_CHANGED")
       << slowdownFactor;

    sendMessage(message);
}

void QDeclarativeObserverService::reloaded()
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("RELOADED");

    sendMessage(message);
}

void QDeclarativeObserverService::setShowAppOnTop(bool showAppOnTop)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SHOW_APP_ON_TOP") << showAppOnTop;

    sendMessage(message);
}

void QDeclarativeObserverService::selectedColorChanged(const QColor &color)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("COLOR_CHANGED")
       << color;

    sendMessage(message);
}

void QDeclarativeObserverService::contextPathUpdated(const QStringList &contextPath)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CONTEXT_PATH_UPDATED")
       << contextPath;

    sendMessage(message);
}

QString QDeclarativeObserverService::idStringForObject(QObject *obj) const
{
    int id = idForObject(obj);
    QString idString = m_stringIdForObjectId.value(id, QString());
    return idString;
}

void QDeclarativeObserverService::sendMessage(const QByteArray &message)
{
    if (status() != Enabled)
        return;

    QDeclarativeDebugService::sendMessage(message);
}

} // namespace QmlJSDebugger
