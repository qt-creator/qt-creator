/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "baseenginedebugclient.h"
#include "qmldebugconstants.h"
#include "qpacketprotocol.h"

namespace QmlDebug {

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
    enum Type { Unknown, Basic, Object, List, SignalProperty, Variant };
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

void BaseEngineDebugClient::decode(QDataStream &ds,
                                   ObjectReference &o,
                                   bool simple)
{
    QmlObjectData data;
    ds >> data;
    int parentId = -1;
    // qt > 4.8.3
    if (objectName() != QLatin1String(Constants::QDECLARATIVE_ENGINE))
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
        o.m_children.append(ObjectReference());
        decode(ds, o.m_children.last(), !recur);
    }

    int propCount;
    ds >> propCount;

    for (int ii = 0; ii < propCount; ++ii) {
        QmlObjectProperty data;
        ds >> data;
        PropertyReference prop;
        prop.m_objectDebugId = o.m_debugId;
        prop.m_name = data.name;
        prop.m_binding = data.binding;
        prop.m_hasNotifySignal = data.hasNotifySignal;
        prop.m_valueTypeName = data.valueTypeName;
        switch (data.type) {
        case QmlObjectProperty::Basic:
        case QmlObjectProperty::List:
        case QmlObjectProperty::SignalProperty:
        case QmlObjectProperty::Variant:
        {
            prop.m_value = data.value;
            break;
        }
        case QmlObjectProperty::Object:
        {
            ObjectReference obj;
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

void BaseEngineDebugClient::decode(QDataStream &ds,
                                   QVariantList &o,
                                   bool simple)
{
    int count;
    ds >> count;
    for (int i = 0; i < count; i++) {
        ObjectReference obj;
        decode(ds, obj, simple);
        o << QVariant::fromValue(obj);
    }
}

void BaseEngineDebugClient::decode(QDataStream &ds,
                                   ContextReference &c)
{
    ds >> c.m_name >> c.m_debugId;

    int contextCount;
    ds >> contextCount;

    for (int ii = 0; ii < contextCount; ++ii) {
        c.m_contexts.append(ContextReference());
        decode(ds, c.m_contexts.last());
    }

    int objectCount;
    ds >> objectCount;

    for (int ii = 0; ii < objectCount; ++ii) {
        ObjectReference obj;
        decode(ds, obj, true);
        obj.m_contextDebugId = c.m_debugId;
        c.m_objects << obj;
    }
}

void BaseEngineDebugClient::stateChanged(State state)
{
    emit newState(state);
}

void BaseEngineDebugClient::messageReceived(const QByteArray &data)
{
    QPacket ds(dataStreamVersion(), data);
    int queryId;
    QByteArray type;
    ds >> type >> queryId;

    if (type == "OBJECT_CREATED") {
        int engineId;
        int objectId;
        int parentId;
        ds >> engineId >> objectId >> parentId;
        emit newObject(engineId, objectId, parentId);
    } else if (type == "LIST_ENGINES_R") {
        int count;
        ds >> count;
        QList<EngineReference> engines;
        engines.reserve(count);
        for (int ii = 0; ii < count; ++ii) {
            EngineReference eng;
            ds >> eng.m_name;
            ds >> eng.m_debugId;
            engines << eng;
        }
        emit result(queryId, QVariant::fromValue(engines), type);
    } else if (type == "LIST_OBJECTS_R") {
        ContextReference rootContext;
        if (!ds.atEnd())
            decode(ds, rootContext);
        emit result(queryId, QVariant::fromValue(rootContext), type);
    } else if (type == "FETCH_OBJECT_R") {
        ObjectReference object;
        if (!ds.atEnd())
            decode(ds, object, false);
        emit result(queryId, QVariant::fromValue(object), type);
    } else if (type == "FETCH_OBJECTS_FOR_LOCATION_R") {
        QVariantList objects;
        if (!ds.atEnd())
            decode(ds, objects, false);
        emit result(queryId, objects, type);
    } else if (type == "EVAL_EXPRESSION_R") {;
        QVariant exprResult;
        ds >> exprResult;
        emit result(queryId, exprResult, type);
    } else if (type == "WATCH_PROPERTY_R" ||
               type == "WATCH_OBJECT_R" ||
               type == "WATCH_EXPR_OBJECT_R" ||
               type == "SET_BINDING_R" ||
               type == "RESET_BINDING_R" ||
               type == "SET_METHOD_BODY_R") {
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

BaseEngineDebugClient::BaseEngineDebugClient(const QString &clientName,
                     QmlDebugConnection *conn)
    : QmlDebugClient(clientName, conn),
      m_nextId(1)
{
    setObjectName(clientName);
}

quint32 BaseEngineDebugClient::addWatch(const PropertyReference &property)
{
    quint32 id = 0;
    if (state() == Enabled) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("WATCH_PROPERTY") << id << property.m_objectDebugId
           << property.m_name.toUtf8();
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::addWatch(const ContextReference &/*context*/,
                                       const QString &/*id*/)
{
    qWarning("QmlEngineDebugClient::addWatch(): Not implemented");
    return 0;
}

quint32 BaseEngineDebugClient::addWatch(const ObjectReference &object,
                                       const QString &expr)
{
    quint32 id = 0;
    if (state() == Enabled) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("WATCH_EXPR_OBJECT") << id << object.m_debugId << expr;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::addWatch(int objectDebugId)
{
    quint32 id = 0;
    if (state() == Enabled) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("WATCH_OBJECT") << id << objectDebugId;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::addWatch(const FileReference &/*file*/)
{
    qWarning("QmlEngineDebugClient::addWatch(): Not implemented");
    return 0;
}

void BaseEngineDebugClient::removeWatch(quint32 id)
{
    if (state() == Enabled) {
        QPacket ds(dataStreamVersion());
        ds << QByteArray("NO_WATCH") << id;
        sendMessage(ds.data());
    }
}

quint32 BaseEngineDebugClient::queryAvailableEngines()
{
    quint32 id = 0;
    if (state() == Enabled) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("LIST_ENGINES") << id;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::queryRootContexts(const EngineReference &engine)
{
    quint32 id = 0;
    if (state() == Enabled && engine.m_debugId != -1) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("LIST_OBJECTS") << id << engine.m_debugId;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::queryObject(int objectId)
{
    quint32 id = 0;
    if (state() == Enabled && objectId != -1) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("FETCH_OBJECT") << id << objectId << false <<
              true;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::queryObjectRecursive(int objectId)
{
    quint32 id = 0;
    if (state() == Enabled && objectId != -1) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("FETCH_OBJECT") << id << objectId << true <<
              true;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::queryExpressionResult(int objectDebugId,
                                                     const QString &expr,
                                                     int engineId)
{
    quint32 id = 0;
    if (state() == Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("EVAL_EXPRESSION") << id << objectDebugId << expr
           << engineId;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::setBindingForObject(
        int objectDebugId,
        const QString &propertyName,
        const QVariant &bindingExpression,
        bool isLiteralValue,
        QString source, int line)
{
    quint32 id = 0;
    if (state() == Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("SET_BINDING") << id << objectDebugId << propertyName
           << bindingExpression << isLiteralValue << source << line;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::resetBindingForObject(
        int objectDebugId,
        const QString &propertyName)
{
    quint32 id = 0;
    if (state() == Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("RESET_BINDING") << id << objectDebugId << propertyName;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::setMethodBody(
        int objectDebugId, const QString &methodName,
        const QString &methodBody)
{
    quint32 id = 0;
    if (state() == Enabled && objectDebugId != -1) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("SET_METHOD_BODY") << id << objectDebugId
           << methodName << methodBody;
        sendMessage(ds.data());
    }
    return id;
}

quint32 BaseEngineDebugClient::queryObjectsForLocation(
        const QString &fileName, int lineNumber, int columnNumber)
{
    quint32 id = 0;
    if (state() == Enabled) {
        id = getId();
        QPacket ds(dataStreamVersion());
        ds << QByteArray("FETCH_OBJECTS_FOR_LOCATION") << id <<
              fileName << lineNumber << columnNumber << false <<
              true;
        sendMessage(ds.data());
    }
    return id;
}

} // namespace QmlDebug
