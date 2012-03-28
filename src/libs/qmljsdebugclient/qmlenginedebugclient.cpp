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

#include "qmlenginedebugclient.h"

const float CURRENT_SUPPORTED_VERSION = 2.0;

namespace QmlJsDebugClient {

struct QmlObjectData {
    QUrl url;
    int lineNumber;
    int columnNumber;
    QString idString;
    QString objectName;
    QString objectType;
    int objectId;
    int contextId;
};

QDataStream &operator>>(QDataStream &ds, QmlObjectData &data)
{
    ds >> data.url >> data.lineNumber >> data.columnNumber >> data.idString
       >> data.objectName >> data.objectType >> data.objectId >> data.contextId;
    return ds;
}

struct QmlObjectProperty {
    enum Type { Unknown, Basic, Object, List, SignalProperty };
    Type type;
    QString name;
    QVariant value;
    QString valueTypeName;
    QString binding;
    bool hasNotifySignal;
};

QDataStream &operator>>(QDataStream &ds, QmlObjectProperty &data)
{
    int type;
    ds >> type >> data.name >> data.value >> data.valueTypeName
       >> data.binding >> data.hasNotifySignal;
    data.type = (QmlObjectProperty::Type)type;
    return ds;
}

void QmlEngineDebugClient::decode(QDataStream &ds,
                                   QmlDebugObjectReference &o,
                                   bool simple)
{
    QmlObjectData data;
    ds >> data;
    int parentId = -1;
    if (objectName() == QLatin1String("QmlDebugger") &&
            serviceVersion() >= CURRENT_SUPPORTED_VERSION )
        ds >> parentId;
    o.m_debugId = data.objectId;
    o.m_className = data.objectType;
    o.m_idString = data.idString;
    o.m_name = data.objectName;
    o.m_source.m_url = data.url;
    o.m_source.m_lineNumber = data.lineNumber;
    o.m_source.m_columnNumber = data.columnNumber;
    o.m_contextDebugId = data.contextId;
    o.m_needsMoreData = simple;
    o.m_parentId = parentId;

    if (simple)
        return;

    int childCount;
    bool recur;
    ds >> childCount >> recur;

    for (int ii = 0; ii < childCount; ++ii) {
        o.m_children.append(QmlDebugObjectReference());
        decode(ds, o.m_children.last(), !recur);
    }

    int propCount;
    ds >> propCount;

    for (int ii = 0; ii < propCount; ++ii) {
        QmlObjectProperty data;
        ds >> data;
        QmlDebugPropertyReference prop;
        prop.m_objectDebugId = o.m_debugId;
        prop.m_name = data.name;
        prop.m_binding = data.binding;
        prop.m_hasNotifySignal = data.hasNotifySignal;
        prop.m_valueTypeName = data.valueTypeName;
        switch (data.type) {
        case QmlObjectProperty::Basic:
        case QmlObjectProperty::List:
        case QmlObjectProperty::SignalProperty:
        {
            prop.m_value = data.value;
            break;
        }
        case QmlObjectProperty::Object:
        {
            QmlDebugObjectReference obj;
            obj.m_debugId = prop.m_value.toInt();
            prop.m_value = qVariantFromValue(obj);
            break;
        }
        case QmlObjectProperty::Unknown:
            break;
        }
        o.m_properties << prop;
    }
}

void QmlEngineDebugClient::decode(QDataStream &ds,
                                   QmlDebugContextReference &c)
{
    ds >> c.m_name >> c.m_debugId;

    int contextCount;
    ds >> contextCount;

    for (int ii = 0; ii < contextCount; ++ii) {
        c.m_contexts.append(QmlDebugContextReference());
        decode(ds, c.m_contexts.last());
    }

    int objectCount;
    ds >> objectCount;

    for (int ii = 0; ii < objectCount; ++ii) {
        QmlDebugObjectReference obj;
        decode(ds, obj, true);
        obj.m_contextDebugId = c.m_debugId;
        c.m_objects << obj;
    }
}

void QmlEngineDebugClient::statusChanged(Status status)
{
    emit newStatus(status);
}

void QmlEngineDebugClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    int queryId;
    QByteArray type;
    ds >> type;

    if (type == "OBJECT_CREATED") {
        emit newObjects();
        return;
    }

    ds >> queryId;

    if (type == "LIST_ENGINES_R") {
        int count;
        ds >> count;
        QmlDebugEngineReferenceList engines;
        for (int ii = 0; ii < count; ++ii) {
            QmlDebugEngineReference eng;
            ds >> eng.m_name;
            ds >> eng.m_debugId;
            engines << eng;
        }
        emit result(queryId, QVariant::fromValue(engines), type);
    } else if (type == "LIST_OBJECTS_R") {
        QmlDebugContextReference rootContext;
        if (!ds.atEnd())
            decode(ds, rootContext);
        emit result(queryId, QVariant::fromValue(rootContext), type);
    } else if (type == "FETCH_OBJECT_R") {
        QmlDebugObjectReference object;
        if (!ds.atEnd())
            decode(ds, object, false);
        emit result(queryId, QVariant::fromValue(object), type);
    } else if (type == "EVAL_EXPRESSION_R") {;
        QVariant exprResult;
        ds >> exprResult;
        emit result(queryId, exprResult, type);
    } else if (type == "WATCH_PROPERTY_R" ||
               type == "WATCH_OBJECT_R" ||
               type == "WATCH_EXPR_OBJECT_R") {
        bool valid;
        ds >> valid;
        emit result(queryId, valid, type);
    } else if (type == "UPDATE_WATCH") {
        int debugId;
        QByteArray name;
        QVariant value;
        ds >> debugId >> name >> value;
        emit valueChanged(debugId, name, value);
    }
}

QmlEngineDebugClient::QmlEngineDebugClient(const QString &clientName,
                     QDeclarativeDebugConnection *conn)
    : QDeclarativeDebugClient(clientName, conn),
      m_nextId(1)
{
    setObjectName(clientName);
}

quint32 QmlEngineDebugClient::addWatch(const QmlDebugPropertyReference &property)
{
    quint32 id;
    if (status() == QDeclarativeDebugClient::Enabled) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_PROPERTY") << id << property.m_objectDebugId
           << property.m_name.toUtf8();
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::addWatch(const QmlDebugContextReference &/*context*/,
                                       const QString &/*id*/)
{
    qWarning("QmlEngineDebugClient::addWatch(): Not implemented");
    return 0;
}

quint32 QmlEngineDebugClient::addWatch(const QmlDebugObjectReference &object,
                                       const QString &expr)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_EXPR_OBJECT") << id << object.m_debugId << expr;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::addWatch(const QmlDebugObjectReference &object)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_OBJECT") << id << object.m_debugId;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::addWatch(const QmlDebugFileReference &/*file*/)
{
    qWarning("QmlEngineDebugClient::addWatch(): Not implemented");
    return 0;
}

void QmlEngineDebugClient::removeWatch(quint32 id)
{
    if (status() == QDeclarativeDebugClient::Enabled) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("NO_WATCH") << id;
        sendMessage(message);
    }
}

quint32 QmlEngineDebugClient::queryAvailableEngines()
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("LIST_ENGINES") << id;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::queryRootContexts(const QmlDebugEngineReference &engine)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled && engine.m_debugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("LIST_OBJECTS") << id << engine.m_debugId;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::queryObject(const QmlDebugObjectReference &object)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled && object.m_debugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("FETCH_OBJECT") << id << object.m_debugId << false <<
              true;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::queryObjectRecursive(const QmlDebugObjectReference &object)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled && object.m_debugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("FETCH_OBJECT") << id << object.m_debugId << true <<
              true;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::queryExpressionResult(int objectDebugId,
                                                    const QString &expr)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("EVAL_EXPRESSION") << id << objectDebugId << expr;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::setBindingForObject(
        int objectDebugId,
        const QString &propertyName,
        const QVariant &bindingExpression,
        bool isLiteralValue,
        QString source, int line)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_BINDING") << objectDebugId << propertyName
           << bindingExpression << isLiteralValue << source << line;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::resetBindingForObject(
        int objectDebugId,
        const QString &propertyName)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("RESET_BINDING") << objectDebugId << propertyName;
        sendMessage(message);
    }
    return id;
}

quint32 QmlEngineDebugClient::setMethodBody(
        int objectDebugId, const QString &methodName,
        const QString &methodBody)
{
    quint32 id = 0;
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        id = getId();
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_METHOD_BODY") << objectDebugId
           << methodName << methodBody;
        sendMessage(message);
    }
    return id;
}

} // namespace QmlJsDebugClient
