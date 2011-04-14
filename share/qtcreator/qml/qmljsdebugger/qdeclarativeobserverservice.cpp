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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qdeclarativeobserverservice.h"

#include <observerprotocol.h>

#include <QStringList>
#include <QColor>

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

    ObserverProtocol::Message type;
    ds >> type;

    switch (type) {
    case ObserverProtocol::SetCurrentObjects: {
        int itemCount = 0;
        ds >> itemCount;

        QList<QObject*> selectedObjects;
        for (int i = 0; i < itemCount; ++i) {
            int debugId = -1;
            ds >> debugId;
            QObject *obj = objectForId(debugId);

            if (obj)
                selectedObjects << obj;
        }

        emit currentObjectsChanged(selectedObjects);
        break;
    }
    case ObserverProtocol::Reload: {
        emit reloadRequested();
        break;
    }
    case ObserverProtocol::SetAnimationSpeed: {
        qreal speed;
        ds >> speed;
        emit animationSpeedChangeRequested(speed);
        break;
    }
    case ObserverProtocol::SetAnimationPaused: {
        bool paused;
        ds >> paused;
        emit executionPauseChangeRequested(paused);
        break;
    }
    case ObserverProtocol::ChangeTool: {
        ObserverProtocol::Tool tool;
        ds >> tool;
        switch (tool) {
        case ObserverProtocol::ColorPickerTool:
            emit colorPickerToolRequested();
            break;
        case ObserverProtocol::SelectTool:
            emit selectToolRequested();
            break;
        case ObserverProtocol::SelectMarqueeTool:
            emit selectMarqueeToolRequested();
            break;
        case ObserverProtocol::ZoomTool:
            emit zoomToolRequested();
            break;
        default:
            qWarning() << "Warning: Unhandled tool:" << tool;
        }
        break;
    }
    case ObserverProtocol::SetDesignMode: {
        bool inDesignMode;
        ds >> inDesignMode;
        emit designModeBehaviorChanged(inDesignMode);
        break;
    }
    case ObserverProtocol::ShowAppOnTop: {
        bool showOnTop;
        ds >> showOnTop;
        emit showAppOnTopChanged(showOnTop);
        break;
    }
    case ObserverProtocol::CreateObject: {
        QString qml;
        int parentId;
        QString filename;
        QStringList imports;
        ds >> qml >> parentId >> imports >> filename;
        emit objectCreationRequested(qml, objectForId(parentId), imports, filename);
        break;
    }
    case ObserverProtocol::DestroyObject: {
        int debugId;
        ds >> debugId;
        if (QObject* obj = objectForId(debugId))
            obj->deleteLater();
        break;
    }
    case ObserverProtocol::MoveObject: {
        int debugId, newParent;
        ds >> debugId >> newParent;
        emit objectReparentRequested(objectForId(debugId), objectForId(newParent));
        break;
    }
    case ObserverProtocol::ObjectIdList: {
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
    case ObserverProtocol::SetContextPathIdx: {
        int contextPathIndex;
        ds >> contextPathIndex;
        emit contextPathIndexChanged(contextPathIndex);
        break;
    }
    case ObserverProtocol::ClearComponentCache: {
        emit clearComponentCacheRequested();
        break;
    }
    default:
        qWarning() << "Warning: Not handling message:" << type;
    }
}

void QDeclarativeObserverService::setDesignModeBehavior(bool inDesignMode)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::SetDesignMode
       << inDesignMode;

    sendMessage(message);
}

void QDeclarativeObserverService::setCurrentObjects(QList<QObject*> objects)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::CurrentObjectsChanged
       << objects.length();

    foreach (QObject *object, objects) {
        int id = idForObject(object);
        ds << id;
    }

    sendMessage(message);
}

void QDeclarativeObserverService::setCurrentTool(QmlJSDebugger::Constants::DesignTool toolId)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::ToolChanged
       << toolId;

    sendMessage(message);
}

void QDeclarativeObserverService::setAnimationSpeed(qreal slowDownFactor)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::AnimationSpeedChanged
       << slowDownFactor;

    sendMessage(message);
}

void QDeclarativeObserverService::setAnimationPaused(bool paused)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::AnimationPausedChanged
       << paused;

    sendMessage(message);
}

void QDeclarativeObserverService::reloaded()
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::Reloaded;

    sendMessage(message);
}

void QDeclarativeObserverService::setShowAppOnTop(bool showAppOnTop)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::ShowAppOnTop << showAppOnTop;

    sendMessage(message);
}

void QDeclarativeObserverService::selectedColorChanged(const QColor &color)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::ColorChanged
       << color;

    sendMessage(message);
}

void QDeclarativeObserverService::contextPathUpdated(const QStringList &contextPath)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << ObserverProtocol::ContextPathUpdated
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
