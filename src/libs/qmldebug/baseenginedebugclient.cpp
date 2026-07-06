// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseenginedebugclient.h"
#include "qmldebugconstants.h"
#include "qpacketprotocol.h"

namespace QmlDebug {

// Cap on the nesting depth of a decoded object or context tree.
// ObjectReference/ContextReference are recursive value types, so building,
// walking or *destroying* an overly deep tree would overflow the call stack
// regardless of how carefully it was decoded. The cap is far beyond any
// realistic QML nesting yet well below that limit. See QTCREATORBUG-33434.
static const int MaxDecodedDepth = 4096;

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
    // The wire format for one object is:
    //   header, [childCount, recur, <each child object>], [propCount, <properties>]
    // where a non-simple object's properties follow its entire child subtree.
    // Decode this iteratively with an explicit stack instead of recursing, so a
    // deeply nested object tree cannot overflow the call stack and crash Qt
    // Creator (QTCREATORBUG-33434).

    const auto readHeader = [&ds](ObjectReference &obj, bool simpleObj) {
        QmlObjectData data;
        int parentId = -1;
        ds >> data >> parentId;
        obj.m_debugId = data.objectId;
        obj.m_className = data.objectType;
        obj.m_idString = data.idString;
        obj.m_name = data.objectName;
        obj.m_source.m_url = data.url;
        obj.m_source.m_lineNumber = data.lineNumber;
        obj.m_source.m_columnNumber = data.columnNumber;
        obj.m_contextDebugId = data.contextId;
        obj.m_needsMoreData = simpleObj;
        obj.m_parentId = parentId;
    };

    const auto readProperties = [&ds](ObjectReference &obj) {
        int propCount = 0;
        ds >> propCount;
        for (int ii = 0; ii < propCount && ds.status() == QDataStream::Ok; ++ii) {
            QmlObjectProperty data;
            ds >> data;
            PropertyReference prop;
            prop.m_objectDebugId = obj.m_debugId;
            prop.m_name = data.name;
            prop.m_binding = data.binding;
            prop.m_hasNotifySignal = data.hasNotifySignal;
            prop.m_valueTypeName = data.valueTypeName;
            switch (data.type) {
            case QmlObjectProperty::Basic:
            case QmlObjectProperty::List:
            case QmlObjectProperty::SignalProperty:
            case QmlObjectProperty::Variant:
                prop.m_value = data.value;
                break;
            case QmlObjectProperty::Object: {
                ObjectReference obj;
                obj.m_debugId = prop.m_value.toInt();
                prop.m_value = QVariant::fromValue(obj);
                break;
            }
            case QmlObjectProperty::Unknown:
                break;
            }
            obj.m_properties << prop;
        }
    };

    readHeader(o, simple);
    if (simple)
        return;

    struct Frame { ObjectReference obj; int childrenLeft; bool recur; };
    QList<Frame> stack;
    {
        int childCount = 0;
        bool recur = false;
        ds >> childCount >> recur;
        stack.append(Frame{std::move(o), childCount, recur});
    }

    bool capped = false;
    while (!stack.isEmpty()) {
        if (!capped && stack.last().childrenLeft > 0 && ds.status() == QDataStream::Ok) {
            if (stack.size() >= MaxDecodedDepth) {
                qWarning("QmlDebug: aborting object decode at depth %d; "
                         "tree too deep or malformed.", MaxDecodedDepth);
                capped = true;
                continue;
            }
            stack.last().childrenLeft--;
            const bool childRecur = stack.last().recur;
            ObjectReference child;
            readHeader(child, !childRecur);
            if (!childRecur) {
                // Simple child: header only, no nested children or properties.
                stack.last().obj.m_children.append(std::move(child));
            } else {
                int childCount = 0;
                bool recur = false;
                ds >> childCount >> recur;
                stack.append(Frame{std::move(child), childCount, recur});
            }
        } else {
            Frame done = std::move(stack.last());
            stack.removeLast();
            // Skip further stream reads once capped: unwind and assemble the
            // partial tree so the output is still well-formed.
            if (!capped)
                readProperties(done.obj);
            if (stack.isEmpty())
                o = std::move(done.obj);
            else
                stack.last().obj.m_children.append(std::move(done.obj));
        }
    }
}

void BaseEngineDebugClient::decode(QDataStream &ds,
                                   QVariantList &o,
                                   bool simple)
{
    int count = 0;
    ds >> count;
    for (int i = 0; i < count && ds.status() == QDataStream::Ok; i++) {
        ObjectReference obj;
        decode(ds, obj, simple);
        o << QVariant::fromValue(obj);
    }
}

void BaseEngineDebugClient::decode(QDataStream &ds,
                                   ContextReference &c)
{
    // Wire format per context: name, debugId, contextCount, <each child
    // context>, objectCount, <each object (simple)>. Decode iteratively with an
    // explicit stack rather than recursing over child contexts, so a deeply
    // nested context tree cannot overflow the call stack (QTCREATORBUG-33434).

    struct Frame { ContextReference ctx; int contextsLeft; };
    QList<Frame> stack;
    {
        ContextReference root;
        int contextCount = 0;
        ds >> root.m_name >> root.m_debugId >> contextCount;
        stack.append(Frame{std::move(root), contextCount});
    }

    bool capped = false;
    while (!stack.isEmpty()) {
        if (!capped && stack.last().contextsLeft > 0 && !ds.atEnd()
                && ds.status() == QDataStream::Ok) {
            if (stack.size() >= MaxDecodedDepth) {
                qWarning("QmlDebug: aborting context decode at depth %d; "
                         "tree too deep or malformed.", MaxDecodedDepth);
                capped = true;
                continue;
            }
            stack.last().contextsLeft--;
            ContextReference child;
            int contextCount = 0;
            ds >> child.m_name >> child.m_debugId >> contextCount;
            stack.append(Frame{std::move(child), contextCount});
        } else {
            Frame done = std::move(stack.last());
            stack.removeLast();

            // Skip further stream reads once capped: unwind and assemble the
            // partial tree so the output is still well-formed.
            if (!capped) {
                int objectCount = 0;
                ds >> objectCount;
                for (int ii = 0; ii < objectCount && !ds.atEnd()
                         && ds.status() == QDataStream::Ok; ++ii) {
                    ObjectReference obj;
                    decode(ds, obj, true);
                    obj.m_contextDebugId = done.ctx.m_debugId;
                    done.ctx.m_objects << obj;
                }
            }

            if (stack.isEmpty())
                c = std::move(done.ctx);
            else
                stack.last().ctx.m_contexts.append(std::move(done.ctx));
        }
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
