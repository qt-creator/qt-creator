#include "qdeclarativedesigndebugserver.h"

#include <QStringList>
#include <QColor>

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

void QDeclarativeDesignDebugServer::setDesignModeBehavior(bool inDesignMode)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("SET_DESIGN_MODE")
       << inDesignMode;

    sendMessage(message);
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

void QDeclarativeDesignDebugServer::reloaded()
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("RELOADED");

    sendMessage(message);
}

void QDeclarativeDesignDebugServer::selectedColorChanged(const QColor &color)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("COLOR_CHANGED")
       << color;

    sendMessage(message);
}

void QDeclarativeDesignDebugServer::contextPathUpdated(const QStringList &contextPath)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ds << QByteArray("CONTEXT_PATH_UPDATED")
       << contextPath;

    sendMessage(message);
}

QString QDeclarativeDesignDebugServer::idStringForObject(QObject *obj) const
{
    int id = idForObject(obj);
    QString idString = m_stringIdForObjectId.value(id, QString());
    return idString;
}
