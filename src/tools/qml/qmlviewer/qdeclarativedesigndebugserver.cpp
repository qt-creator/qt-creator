#include "qdeclarativedesigndebugserver.h"

#include <QDebug>

QDeclarativeDesignDebugServer::QDeclarativeDesignDebugServer(QObject *parent)
    : QDeclarativeDebugService(QLatin1String("QDeclarativeDesignMode"), parent)
{
}

void QDeclarativeDesignDebugServer::messageReceived(const QByteArray &message)
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
            colorPickerToolRequested();
        } else if (toolName == "SELECT") {
            selectToolRequested();
        } else if (toolName == "SELECT_MARQUEE") {
            selectMarqueeToolRequested();
        } else if (toolName == "ZOOM") {
            zoomToolRequested();
        }
    }
}


void QDeclarativeDesignDebugServer::setCurrentObjects(QList<QObject*> objects)
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

void QDeclarativeDesignDebugServer::setCurrentTool(QmlViewer::Constants::DesignTool toolId)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("TOOL_CHANGED")
       << toolId;

    sendMessage(message);
}

void QDeclarativeDesignDebugServer::setAnimationSpeed(qreal slowdownFactor)
{

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("ANIMATION_SPEED_CHANGED")
       << slowdownFactor;

    sendMessage(message);
}
