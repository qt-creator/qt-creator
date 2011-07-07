/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qdeclarativeinspectorservice.h"

#include <inspectorprotocol.h>

#include <QStringList>
#include <QColor>

namespace QmlJSDebugger {

Q_GLOBAL_STATIC(QDeclarativeInspectorService, serviceInstance)

QDeclarativeInspectorService::QDeclarativeInspectorService()
    : QDeclarativeDebugService(QLatin1String("QDeclarativeObserverMode"))
{
}

QDeclarativeInspectorService *QDeclarativeInspectorService::instance()
{
    return serviceInstance();
}

void QDeclarativeInspectorService::statusChanged(Status status)
{
    emit debuggingClientChanged((status == Enabled));
}

void QDeclarativeInspectorService::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    InspectorProtocol::Message type;
    ds >> type;

    switch (type) {
    case InspectorProtocol::SetCurrentObjects: {
        int itemCount = 0;
        ds >> itemCount;

        QList<QObject*> selectedObjects;
        for (int i = 0; i < itemCount; ++i) {
            int debugId = -1;
            ds >> debugId;
            if (QObject *obj = objectForId(debugId))
                selectedObjects << obj;
        }

        emit currentObjectsChanged(selectedObjects);
        break;
    }
    case InspectorProtocol::Reload: {
        emit reloadRequested();
        break;
    }
    case InspectorProtocol::SetAnimationSpeed: {
        qreal speed;
        ds >> speed;
        emit animationSpeedChangeRequested(speed);
        break;
    }
    case InspectorProtocol::SetAnimationPaused: {
        bool paused;
        ds >> paused;
        emit executionPauseChangeRequested(paused);
        break;
    }
    case InspectorProtocol::ChangeTool: {
        InspectorProtocol::Tool tool;
        ds >> tool;
        switch (tool) {
        case InspectorProtocol::ColorPickerTool:
            emit colorPickerToolRequested();
            break;
        case InspectorProtocol::SelectTool:
            emit selectToolRequested();
            break;
        case InspectorProtocol::SelectMarqueeTool:
            emit selectMarqueeToolRequested();
            break;
        case InspectorProtocol::ZoomTool:
            emit zoomToolRequested();
            break;
        default:
            qWarning() << "Warning: Unhandled tool:" << tool;
        }
        break;
    }
    case InspectorProtocol::SetDesignMode: {
        bool inDesignMode;
        ds >> inDesignMode;
        emit designModeBehaviorChanged(inDesignMode);
        break;
    }
    case InspectorProtocol::ShowAppOnTop: {
        bool showOnTop;
        ds >> showOnTop;
        emit showAppOnTopChanged(showOnTop);
        break;
    }
    case InspectorProtocol::CreateObject: {
        QString qml;
        int parentId;
        QString filename;
        QStringList imports;
        ds >> qml >> parentId >> imports >> filename;
        int order = -1;
        if (!ds.atEnd()) {
            ds >> order;
        }
        emit objectCreationRequested(qml, objectForId(parentId), imports, filename, order);
        break;
    }
    case InspectorProtocol::DestroyObject: {
        int debugId;
        ds >> debugId;
        if (QObject* obj = objectForId(debugId)) {
            emit objectDeletionRequested(obj);
        }
        break;
    }
    case InspectorProtocol::MoveObject: {
        int debugId, newParent;
        ds >> debugId >> newParent;
        emit objectReparentRequested(objectForId(debugId), objectForId(newParent));
        break;
    }
    case InspectorProtocol::ObjectIdList: {
        int itemCount;
        ds >> itemCount;
        m_stringIdForObjectId.clear();
        for (int i = 0; i < itemCount; ++i) {
            int itemDebugId;
            QString itemIdString;
            ds >> itemDebugId
               >> itemIdString;

            m_stringIdForObjectId.insert(itemDebugId, itemIdString);
        }
        break;
    }
    case InspectorProtocol::ClearComponentCache: {
        emit clearComponentCacheRequested();
        break;
    }
    default:
        qWarning() << "Warning: Not handling message:" << type;
    }
}

void QDeclarativeInspectorService::setDesignModeBehavior(bool inDesignMode)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::SetDesignMode
       << inDesignMode;

    sendMessage(message);
}

void QDeclarativeInspectorService::setCurrentObjects(QList<QObject*> objects)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::CurrentObjectsChanged
       << objects.length();

    foreach (QObject *object, objects) {
        int id = idForObject(object);
        ds << id;
    }

    sendMessage(message);
}

void QDeclarativeInspectorService::setCurrentTool(QmlJSDebugger::Constants::DesignTool toolId)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::ToolChanged
       << toolId;

    sendMessage(message);
}

void QDeclarativeInspectorService::setAnimationSpeed(qreal slowDownFactor)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::AnimationSpeedChanged
       << slowDownFactor;

    sendMessage(message);
}

void QDeclarativeInspectorService::setAnimationPaused(bool paused)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::AnimationPausedChanged
       << paused;

    sendMessage(message);
}

void QDeclarativeInspectorService::reloaded()
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::Reloaded;

    sendMessage(message);
}

void QDeclarativeInspectorService::setShowAppOnTop(bool showAppOnTop)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::ShowAppOnTop << showAppOnTop;

    sendMessage(message);
}

void QDeclarativeInspectorService::selectedColorChanged(const QColor &color)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << InspectorProtocol::ColorChanged
       << color;

    sendMessage(message);
}

QString QDeclarativeInspectorService::idStringForObject(QObject *obj) const
{
    int id = idForObject(obj);
    QString idString = m_stringIdForObjectId.value(id, QString());
    return idString;
}

void QDeclarativeInspectorService::sendMessage(const QByteArray &message)
{
    if (status() != Enabled)
        return;

    QDeclarativeDebugService::sendMessage(message);
}

} // namespace QmlJSDebugger
