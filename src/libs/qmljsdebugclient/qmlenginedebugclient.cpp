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

namespace QmlJsDebugClient {

class QmlEngineDebugClientPrivate
{
public:
    QmlEngineDebugClientPrivate(QmlEngineDebugClient *);
    ~QmlEngineDebugClientPrivate();

    void statusChanged(QDeclarativeDebugClient::Status status);
    void message(const QByteArray &);

    QmlEngineDebugClient *q;
    int nextId;
    int getId();

    void decode(QDataStream &, QDeclarativeDebugContextReference &);
    void decode(QDataStream &, QDeclarativeDebugObjectReference &, bool simple);

    static void remove(QmlEngineDebugClient *, QDeclarativeDebugEnginesQuery *);
    static void remove(QmlEngineDebugClient *, QDeclarativeDebugRootContextQuery *);
    static void remove(QmlEngineDebugClient *, QDeclarativeDebugObjectQuery *);
    static void remove(QmlEngineDebugClient *, QDeclarativeDebugExpressionQuery *);
    static void remove(QmlEngineDebugClient *, QDeclarativeDebugWatch *);

    QHash<int, QDeclarativeDebugEnginesQuery *> enginesQuery;
    QHash<int, QDeclarativeDebugRootContextQuery *> rootContextQuery;
    QHash<int, QDeclarativeDebugObjectQuery *> objectQuery;
    QHash<int, QDeclarativeDebugExpressionQuery *> expressionQuery;

    QHash<int, QDeclarativeDebugWatch *> watched;
};

void QmlEngineDebugClient::statusChanged(Status status)
{
    emit newStatus(status);
}

void QmlEngineDebugClient::messageReceived(const QByteArray &data)
{
    d->message(data);
}

QmlEngineDebugClientPrivate::QmlEngineDebugClientPrivate(QmlEngineDebugClient *q)
    : q(q), nextId(0)
{
}

QmlEngineDebugClientPrivate::~QmlEngineDebugClientPrivate()
{
    QHash<int, QDeclarativeDebugEnginesQuery*>::iterator enginesIter = enginesQuery.begin();
    for (; enginesIter != enginesQuery.end(); ++enginesIter) {
        enginesIter.value()->m_client = 0;
        if (enginesIter.value()->state() == QDeclarativeDebugQuery::Waiting)
            enginesIter.value()->setState(QDeclarativeDebugQuery::Error);
    }

    QHash<int, QDeclarativeDebugRootContextQuery*>::iterator rootContextIter = rootContextQuery.begin();
    for (; rootContextIter != rootContextQuery.end(); ++rootContextIter) {
        rootContextIter.value()->m_client = 0;
        if (rootContextIter.value()->state() == QDeclarativeDebugQuery::Waiting)
            rootContextIter.value()->setState(QDeclarativeDebugQuery::Error);
    }

    QHash<int, QDeclarativeDebugObjectQuery*>::iterator objectIter = objectQuery.begin();
    for (; objectIter != objectQuery.end(); ++objectIter) {
        objectIter.value()->m_client = 0;
        if (objectIter.value()->state() == QDeclarativeDebugQuery::Waiting)
            objectIter.value()->setState(QDeclarativeDebugQuery::Error);
    }

    QHash<int, QDeclarativeDebugExpressionQuery*>::iterator exprIter = expressionQuery.begin();
    for (; exprIter != expressionQuery.end(); ++exprIter) {
        exprIter.value()->m_client = 0;
        if (exprIter.value()->state() == QDeclarativeDebugQuery::Waiting)
            exprIter.value()->setState(QDeclarativeDebugQuery::Error);
    }

    QHash<int, QDeclarativeDebugWatch*>::iterator watchIter = watched.begin();
    for (; watchIter != watched.end(); ++watchIter) {
        watchIter.value()->m_client = 0;
        watchIter.value()->setState(QDeclarativeDebugWatch::Dead);
    }
}

int QmlEngineDebugClientPrivate::getId()
{
    return nextId++;
}

void QmlEngineDebugClientPrivate::remove(QmlEngineDebugClient *c, QDeclarativeDebugEnginesQuery *q)
{
    if (c && q) {
        QmlEngineDebugClientPrivate *p = c->priv();
        p->enginesQuery.remove(q->m_queryId);
    }
}

void QmlEngineDebugClientPrivate::remove(QmlEngineDebugClient *c,
                                            QDeclarativeDebugRootContextQuery *q)
{
    if (c && q) {
        QmlEngineDebugClientPrivate *p = c->priv();
        p->rootContextQuery.remove(q->m_queryId);
    }
}

void QmlEngineDebugClientPrivate::remove(QmlEngineDebugClient *c, QDeclarativeDebugWatch *w)
{
    if (c && w) {
        QmlEngineDebugClientPrivate *p = c->priv();
        p->watched.remove(w->m_queryId);
    }
}

// from qdeclarativeenginedebug_p.h
struct QDeclarativeObjectData {
    QUrl url;
    int lineNumber;
    int columnNumber;
    QString idString;
    QString objectName;
    QString objectType;
    int objectId;
    int contextId;
};

QDataStream &operator>>(QDataStream &ds, QDeclarativeObjectData &data)
{
    ds >> data.url >> data.lineNumber >> data.columnNumber >> data.idString
       >> data.objectName >> data.objectType >> data.objectId >> data.contextId;
    return ds;
}

struct QDeclarativeObjectProperty {
    enum Type { Unknown, Basic, Object, List, SignalProperty };
    Type type;
    QString name;
    QVariant value;
    QString valueTypeName;
    QString binding;
    bool hasNotifySignal;
};

QDataStream &operator>>(QDataStream &ds, QDeclarativeObjectProperty &data)
{
    int type;
    ds >> type >> data.name >> data.value >> data.valueTypeName
       >> data.binding >> data.hasNotifySignal;
    data.type = (QDeclarativeObjectProperty::Type)type;
    return ds;
}

void QmlEngineDebugClientPrivate::remove(QmlEngineDebugClient *c, QDeclarativeDebugObjectQuery *q)
{
    if (c && q) {
        QmlEngineDebugClientPrivate *p = c->priv();
        p->objectQuery.remove(q->m_queryId);
    }
}

void QmlEngineDebugClientPrivate::remove(QmlEngineDebugClient *c, QDeclarativeDebugExpressionQuery *q)
{
    if (c && q) {
        QmlEngineDebugClientPrivate *p = c->priv();
        p->expressionQuery.remove(q->m_queryId);
    }
}

void QmlEngineDebugClientPrivate::decode(QDataStream &ds, QDeclarativeDebugObjectReference &o,
                                            bool simple)
{
    QDeclarativeObjectData data;
    ds >> data;
    o.m_debugId = data.objectId;
    o.m_class = data.objectType;
    o.m_idString = data.idString;
    o.m_name = data.objectName;
    o.m_source.m_url = data.url;
    o.m_source.m_lineNumber = data.lineNumber;
    o.m_source.m_columnNumber = data.columnNumber;
    o.m_contextDebugId = data.contextId;

    if (simple)
        return;

    int childCount;
    bool recur;
    ds >> childCount >> recur;

    for (int ii = 0; ii < childCount; ++ii) {
        o.m_children.append(QDeclarativeDebugObjectReference());
        decode(ds, o.m_children.last(), !recur);
    }

    int propCount;
    ds >> propCount;

    for (int ii = 0; ii < propCount; ++ii) {
        QDeclarativeObjectProperty data;
        ds >> data;
        QDeclarativeDebugPropertyReference prop;
        prop.m_objectDebugId = o.m_debugId;
        prop.m_name = data.name;
        prop.m_binding = data.binding;
        prop.m_hasNotifySignal = data.hasNotifySignal;
        prop.m_valueTypeName = data.valueTypeName;
        switch (data.type) {
        case QDeclarativeObjectProperty::Basic:
        case QDeclarativeObjectProperty::List:
        case QDeclarativeObjectProperty::SignalProperty:
        {
            prop.m_value = data.value;
            break;
        }
        case QDeclarativeObjectProperty::Object:
        {
            QDeclarativeDebugObjectReference obj;
            obj.m_debugId = prop.m_value.toInt();
            prop.m_value = qVariantFromValue(obj);
            break;
        }
        case QDeclarativeObjectProperty::Unknown:
            break;
        }
        o.m_properties << prop;
    }
}

void QmlEngineDebugClientPrivate::decode(QDataStream &ds, QDeclarativeDebugContextReference &c)
{
    ds >> c.m_name >> c.m_debugId;

    int contextCount;
    ds >> contextCount;

    for (int ii = 0; ii < contextCount; ++ii) {
        c.m_contexts.append(QDeclarativeDebugContextReference());
        decode(ds, c.m_contexts.last());
    }

    int objectCount;
    ds >> objectCount;

    for (int ii = 0; ii < objectCount; ++ii) {
        QDeclarativeDebugObjectReference obj;
        decode(ds, obj, true);

        obj.m_contextDebugId = c.m_debugId;
        c.m_objects << obj;
    }
}

void QmlEngineDebugClientPrivate::statusChanged(QDeclarativeDebugClient::Status status)
{
    emit q->statusChanged(status);
}

void QmlEngineDebugClientPrivate::message(const QByteArray &data)
{
    QDataStream ds(data);

    QByteArray type;
    ds >> type;

    //qDebug() << "QDeclarativeEngineDebugPrivate::message()" << type;

    if (type == "LIST_ENGINES_R") {
        int queryId;
        ds >> queryId;

        QDeclarativeDebugEnginesQuery *query = enginesQuery.value(queryId);
        if (!query)
            return;
        enginesQuery.remove(queryId);

        int count;
        ds >> count;

        for (int ii = 0; ii < count; ++ii) {
            QDeclarativeDebugEngineReference ref;
            ds >> ref.m_name;
            ds >> ref.m_debugId;
            query->m_engines << ref;
        }

        query->m_client = 0;
        query->setState(QDeclarativeDebugQuery::Completed);
    } else if (type == "LIST_OBJECTS_R") {
        int queryId;
        ds >> queryId;

        QDeclarativeDebugRootContextQuery *query = rootContextQuery.value(queryId);
        if (!query)
            return;
        rootContextQuery.remove(queryId);

        if (!ds.atEnd())
            decode(ds, query->m_context);

        query->m_client = 0;
        query->setState(QDeclarativeDebugQuery::Completed);
    } else if (type == "FETCH_OBJECT_R") {
        int queryId;
        ds >> queryId;

        QDeclarativeDebugObjectQuery *query = objectQuery.value(queryId);
        if (!query)
            return;
        objectQuery.remove(queryId);

        if (!ds.atEnd())
            decode(ds, query->m_object, false);

        query->m_client = 0;
        query->setState(QDeclarativeDebugQuery::Completed);
    } else if (type == "EVAL_EXPRESSION_R") {
        int queryId;
        QVariant result;
        ds >> queryId >> result;

        QDeclarativeDebugExpressionQuery *query = expressionQuery.value(queryId);
        if (!query)
            return;
        expressionQuery.remove(queryId);

        query->m_result = result;
        query->m_client = 0;
        query->setState(QDeclarativeDebugQuery::Completed);
    } else if (type == "WATCH_PROPERTY_R") {
        int queryId;
        bool ok;
        ds >> queryId >> ok;

        QDeclarativeDebugWatch *watch = watched.value(queryId);
        if (!watch)
            return;

        watch->setState(ok ? QDeclarativeDebugWatch::Active : QDeclarativeDebugWatch::Inactive);
    } else if (type == "WATCH_OBJECT_R") {
        int queryId;
        bool ok;
        ds >> queryId >> ok;

        QDeclarativeDebugWatch *watch = watched.value(queryId);
        if (!watch)
            return;

        watch->setState(ok ? QDeclarativeDebugWatch::Active : QDeclarativeDebugWatch::Inactive);
    } else if (type == "WATCH_EXPR_OBJECT_R") {
        int queryId;
        bool ok;
        ds >> queryId >> ok;

        QDeclarativeDebugWatch *watch = watched.value(queryId);
        if (!watch)
            return;

        watch->setState(ok ? QDeclarativeDebugWatch::Active : QDeclarativeDebugWatch::Inactive);
    } else if (type == "UPDATE_WATCH") {
        int queryId;
        int debugId;
        QByteArray name;
        QVariant value;
        ds >> queryId >> debugId >> name >> value;

        QDeclarativeDebugWatch *watch = watched.value(queryId, 0);
        if (!watch)
            return;
        emit watch->valueChanged(name, value);
    } else if (type == "OBJECT_CREATED") {
        emit q->newObjects();
    }
}

QmlEngineDebugClient::QmlEngineDebugClient(QDeclarativeDebugConnection *client)
    : QDeclarativeDebugClient(QLatin1String("QDeclarativeEngine"), client),
      d(new QmlEngineDebugClientPrivate(this))
{
}

QmlEngineDebugClient::~QmlEngineDebugClient()
{
    delete d;
}

QDeclarativeDebugPropertyWatch *QmlEngineDebugClient::addWatch(const QDeclarativeDebugPropertyReference &property, QObject *parent)
{
    QDeclarativeDebugPropertyWatch *watch = new QDeclarativeDebugPropertyWatch(parent);
    if (status() == QDeclarativeDebugClient::Enabled) {
        int queryId = d->getId();
        watch->m_queryId = queryId;
        watch->m_client = this;
        watch->m_objectDebugId = property.objectDebugId();
        watch->m_name = property.name();
        d->watched.insert(queryId, watch);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_PROPERTY") << queryId << property.objectDebugId() << property.name().toUtf8();
        sendMessage(message);
    } else {
        watch->m_state = QDeclarativeDebugWatch::Dead;
    }

    return watch;
}

QDeclarativeDebugWatch *QmlEngineDebugClient::addWatch(const QDeclarativeDebugContextReference &, const QString &, QObject *)
{
    qWarning("QDeclarativeEngineDebug::addWatch(): Not implemented");
    return 0;
}

QDeclarativeDebugObjectExpressionWatch *QmlEngineDebugClient::addWatch(const QDeclarativeDebugObjectReference &object, const QString &expr, QObject *parent)
{
    QDeclarativeDebugObjectExpressionWatch *watch = new QDeclarativeDebugObjectExpressionWatch(parent);
    if (status() == QDeclarativeDebugClient::Enabled) {
        int queryId = d->getId();
        watch->m_queryId = queryId;
        watch->m_client = this;
        watch->m_objectDebugId = object.debugId();
        watch->m_expr = expr;
        d->watched.insert(queryId, watch);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_EXPR_OBJECT") << queryId << object.debugId() << expr;
        sendMessage(message);
    } else {
        watch->m_state = QDeclarativeDebugWatch::Dead;
    }
    return watch;
}

QDeclarativeDebugWatch *QmlEngineDebugClient::addWatch(const QDeclarativeDebugObjectReference &object, QObject *parent)
{
    QDeclarativeDebugWatch *watch = new QDeclarativeDebugWatch(parent);
    if (status() == QDeclarativeDebugClient::Enabled) {
        int queryId = d->getId();
        watch->m_queryId = queryId;
        watch->m_client = this;
        watch->m_objectDebugId = object.debugId();
        d->watched.insert(queryId, watch);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("WATCH_OBJECT") << queryId << object.debugId();
        sendMessage(message);
    } else {
        watch->m_state = QDeclarativeDebugWatch::Dead;
    }

    return watch;
}

QDeclarativeDebugWatch *QmlEngineDebugClient::addWatch(const QDeclarativeDebugFileReference &, QObject *)
{
    qWarning("QDeclarativeEngineDebug::addWatch(): Not implemented");
    return 0;
}

void QmlEngineDebugClient::removeWatch(QDeclarativeDebugWatch *watch)
{
    if (!watch || !watch->m_client)
        return;

    watch->m_client = 0;
    watch->setState(QDeclarativeDebugWatch::Inactive);
    
    d->watched.remove(watch->queryId());

    if (status() == QDeclarativeDebugClient::Enabled) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("NO_WATCH") << watch->queryId();
        sendMessage(message);
    }
}

QDeclarativeDebugEnginesQuery *QmlEngineDebugClient::queryAvailableEngines(QObject *parent)
{
    QDeclarativeDebugEnginesQuery *query = new QDeclarativeDebugEnginesQuery(parent);
    if (status() == QDeclarativeDebugClient::Enabled) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->enginesQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("LIST_ENGINES") << queryId;
        sendMessage(message);
    } else {
        query->m_state = QDeclarativeDebugQuery::Error;
    }

    return query;
}

QDeclarativeDebugRootContextQuery *QmlEngineDebugClient::queryRootContexts(const QDeclarativeDebugEngineReference &engine, QObject *parent)
{
    QDeclarativeDebugRootContextQuery *query = new QDeclarativeDebugRootContextQuery(parent);
    if (status() == QDeclarativeDebugClient::Enabled && engine.debugId() != -1) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->rootContextQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("LIST_OBJECTS") << queryId << engine.debugId();
        sendMessage(message);
    } else {
        query->m_state = QDeclarativeDebugQuery::Error;
    }

    return query;
}

QDeclarativeDebugObjectQuery *QmlEngineDebugClient::queryObject(const QDeclarativeDebugObjectReference &object, QObject *parent)
{
    QDeclarativeDebugObjectQuery *query = new QDeclarativeDebugObjectQuery(parent);
    if (status() == QDeclarativeDebugClient::Enabled && object.debugId() != -1) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->objectQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("FETCH_OBJECT") << queryId << object.debugId()
           << false << true;
        sendMessage(message);
    } else {
        query->m_state = QDeclarativeDebugQuery::Error;
    }

    return query;
}

QDeclarativeDebugObjectQuery *QmlEngineDebugClient::queryObjectRecursive(const QDeclarativeDebugObjectReference &object, QObject *parent)
{
    QDeclarativeDebugObjectQuery *query = new QDeclarativeDebugObjectQuery(parent);
    if (status() == QDeclarativeDebugClient::Enabled && object.debugId() != -1) {
        query->m_client = this;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->objectQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("FETCH_OBJECT") << queryId << object.debugId()
           << true << true;
        sendMessage(message);
    } else {
        query->m_state = QDeclarativeDebugQuery::Error;
    }

    return query;
}

QDeclarativeDebugExpressionQuery *QmlEngineDebugClient::queryExpressionResult(int objectDebugId, const QString &expr, QObject *parent)
{
    QDeclarativeDebugExpressionQuery *query = new QDeclarativeDebugExpressionQuery(parent);
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        query->m_client = this;
        query->m_expr = expr;
        int queryId = d->getId();
        query->m_queryId = queryId;
        d->expressionQuery.insert(queryId, query);

        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("EVAL_EXPRESSION") << queryId << objectDebugId << expr;
        sendMessage(message);
    } else {
        query->m_state = QDeclarativeDebugQuery::Error;
    }

    return query;
}

bool QmlEngineDebugClient::setBindingForObject(int objectDebugId, const QString &propertyName,
                                                  const QVariant &bindingExpression,
                                                  bool isLiteralValue,
                                                  QString source, int line)
{
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_BINDING") << objectDebugId << propertyName << bindingExpression << isLiteralValue << source << line;
        sendMessage(message);
        return true;
    } else {
        return false;
    }
}

bool QmlEngineDebugClient::resetBindingForObject(int objectDebugId, const QString &propertyName)
{
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("RESET_BINDING") << objectDebugId << propertyName;
        sendMessage(message);
        return true;
    } else {
        return false;
    }
}

bool QmlEngineDebugClient::setMethodBody(int objectDebugId, const QString &methodName,
                                            const QString &methodBody)
{
    if (status() == QDeclarativeDebugClient::Enabled && objectDebugId != -1) {
        QByteArray message;
        QDataStream ds(&message, QIODevice::WriteOnly);
        ds << QByteArray("SET_METHOD_BODY") << objectDebugId << methodName << methodBody;
        sendMessage(message);
        return true;
    } else {
        return false;
    }
}

QDeclarativeDebugWatch::QDeclarativeDebugWatch(QObject *parent)
    : QObject(parent), m_state(Waiting), m_queryId(-1), m_client(0), m_objectDebugId(-1)
{
}

QDeclarativeDebugWatch::~QDeclarativeDebugWatch()
{
    if (m_client && m_queryId != -1)
        QmlEngineDebugClientPrivate::remove(m_client, this);
}

int QDeclarativeDebugWatch::queryId() const
{
    return m_queryId;
}

int QDeclarativeDebugWatch::objectDebugId() const
{
    return m_objectDebugId;
}

QDeclarativeDebugWatch::State QDeclarativeDebugWatch::state() const
{
    return m_state;
}

void QDeclarativeDebugWatch::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(m_state);
}

QDeclarativeDebugPropertyWatch::QDeclarativeDebugPropertyWatch(QObject *parent)
    : QDeclarativeDebugWatch(parent)
{
}

QString QDeclarativeDebugPropertyWatch::name() const
{
    return m_name;
}


QDeclarativeDebugObjectExpressionWatch::QDeclarativeDebugObjectExpressionWatch(QObject *parent)
    : QDeclarativeDebugWatch(parent)
{
}

QString QDeclarativeDebugObjectExpressionWatch::expression() const
{
    return m_expr;
}


QDeclarativeDebugQuery::QDeclarativeDebugQuery(QObject *parent)
    : QObject(parent), m_state(Waiting)
{
}

QDeclarativeDebugQuery::State QDeclarativeDebugQuery::state() const
{
    return m_state;
}

bool QDeclarativeDebugQuery::isWaiting() const
{
    return m_state == Waiting;
}

void QDeclarativeDebugQuery::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(m_state);
}

QDeclarativeDebugEnginesQuery::QDeclarativeDebugEnginesQuery(QObject *parent)
    : QDeclarativeDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QDeclarativeDebugEnginesQuery::~QDeclarativeDebugEnginesQuery()
{
    if (m_client && m_queryId != -1)
        QmlEngineDebugClientPrivate::remove(m_client, this);
}

QList<QDeclarativeDebugEngineReference> QDeclarativeDebugEnginesQuery::engines() const
{
    return m_engines;
}

QDeclarativeDebugRootContextQuery::QDeclarativeDebugRootContextQuery(QObject *parent)
    : QDeclarativeDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QDeclarativeDebugRootContextQuery::~QDeclarativeDebugRootContextQuery()
{
    if (m_client && m_queryId != -1)
        QmlEngineDebugClientPrivate::remove(m_client, this);
}

QDeclarativeDebugContextReference QDeclarativeDebugRootContextQuery::rootContext() const
{
    return m_context;
}

QDeclarativeDebugObjectQuery::QDeclarativeDebugObjectQuery(QObject *parent)
    : QDeclarativeDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QDeclarativeDebugObjectQuery::~QDeclarativeDebugObjectQuery()
{
    if (m_client && m_queryId != -1)
        QmlEngineDebugClientPrivate::remove(m_client, this);
}

QDeclarativeDebugObjectReference QDeclarativeDebugObjectQuery::object() const
{
    return m_object;
}

QDeclarativeDebugExpressionQuery::QDeclarativeDebugExpressionQuery(QObject *parent)
    : QDeclarativeDebugQuery(parent), m_client(0), m_queryId(-1)
{
}

QDeclarativeDebugExpressionQuery::~QDeclarativeDebugExpressionQuery()
{
    if (m_client && m_queryId != -1)
        QmlEngineDebugClientPrivate::remove(m_client, this);
}

QVariant QDeclarativeDebugExpressionQuery::expression() const
{
    return m_expr;
}

QVariant QDeclarativeDebugExpressionQuery::result() const
{
    return m_result;
}

QDeclarativeDebugEngineReference::QDeclarativeDebugEngineReference()
    : m_debugId(-1)
{
}

QDeclarativeDebugEngineReference::QDeclarativeDebugEngineReference(int debugId)
    : m_debugId(debugId)
{
}

QDeclarativeDebugEngineReference::QDeclarativeDebugEngineReference(const QDeclarativeDebugEngineReference &o)
    : m_debugId(o.m_debugId), m_name(o.m_name)
{
}

QDeclarativeDebugEngineReference &
QDeclarativeDebugEngineReference::operator=(const QDeclarativeDebugEngineReference &o)
{
    m_debugId = o.m_debugId; m_name = o.m_name;
    return *this;
}

int QDeclarativeDebugEngineReference::debugId() const
{
    return m_debugId;
}

QString QDeclarativeDebugEngineReference::name() const
{
    return m_name;
}

QDeclarativeDebugObjectReference::QDeclarativeDebugObjectReference()
    : m_debugId(-1), m_contextDebugId(-1)
{
}

QDeclarativeDebugObjectReference::QDeclarativeDebugObjectReference(int debugId)
    : m_debugId(debugId), m_contextDebugId(-1)
{
}

QDeclarativeDebugObjectReference::QDeclarativeDebugObjectReference(const QDeclarativeDebugObjectReference &o)
    : m_debugId(o.m_debugId), m_class(o.m_class), m_idString(o.m_idString),
      m_name(o.m_name), m_source(o.m_source), m_contextDebugId(o.m_contextDebugId),
      m_properties(o.m_properties), m_children(o.m_children)
{
}

QDeclarativeDebugObjectReference &
QDeclarativeDebugObjectReference::operator=(const QDeclarativeDebugObjectReference &o)
{
    m_debugId = o.m_debugId; m_class = o.m_class; m_idString = o.m_idString;
    m_name = o.m_name; m_source = o.m_source; m_contextDebugId = o.m_contextDebugId;
    m_properties = o.m_properties; m_children = o.m_children;
    return *this;
}

int QDeclarativeDebugObjectReference::debugId() const
{
    return m_debugId;
}

QString QDeclarativeDebugObjectReference::className() const
{
    return m_class;
}

QString QDeclarativeDebugObjectReference::idString() const
{
    return m_idString;
}

QString QDeclarativeDebugObjectReference::name() const
{
    return m_name;
}

QDeclarativeDebugFileReference QDeclarativeDebugObjectReference::source() const
{
    return m_source;
}

int QDeclarativeDebugObjectReference::contextDebugId() const
{
    return m_contextDebugId;
}

QList<QDeclarativeDebugPropertyReference> QDeclarativeDebugObjectReference::properties() const
{
    return m_properties;
}

QList<QDeclarativeDebugObjectReference> QDeclarativeDebugObjectReference::children() const
{
    return m_children;
}

QDeclarativeDebugContextReference::QDeclarativeDebugContextReference()
    : m_debugId(-1)
{
}

QDeclarativeDebugContextReference::QDeclarativeDebugContextReference(const QDeclarativeDebugContextReference &o)
    : m_debugId(o.m_debugId), m_name(o.m_name), m_objects(o.m_objects), m_contexts(o.m_contexts)
{
}

QDeclarativeDebugContextReference &QDeclarativeDebugContextReference::operator=(const QDeclarativeDebugContextReference &o)
{
    m_debugId = o.m_debugId; m_name = o.m_name; m_objects = o.m_objects;
    m_contexts = o.m_contexts;
    return *this;
}

int QDeclarativeDebugContextReference::debugId() const
{
    return m_debugId;
}

QString QDeclarativeDebugContextReference::name() const
{
    return m_name;
}

QList<QDeclarativeDebugObjectReference> QDeclarativeDebugContextReference::objects() const
{
    return m_objects;
}

QList<QDeclarativeDebugContextReference> QDeclarativeDebugContextReference::contexts() const
{
    return m_contexts;
}

QDeclarativeDebugFileReference::QDeclarativeDebugFileReference()
    : m_lineNumber(-1), m_columnNumber(-1)
{
}

QDeclarativeDebugFileReference::QDeclarativeDebugFileReference(const QDeclarativeDebugFileReference &o)
    : m_url(o.m_url), m_lineNumber(o.m_lineNumber), m_columnNumber(o.m_columnNumber)
{
}

QDeclarativeDebugFileReference &QDeclarativeDebugFileReference::operator=(const QDeclarativeDebugFileReference &o)
{
    m_url = o.m_url; m_lineNumber = o.m_lineNumber; m_columnNumber = o.m_columnNumber;
    return *this;
}

QUrl QDeclarativeDebugFileReference::url() const
{
    return m_url;
}

void QDeclarativeDebugFileReference::setUrl(const QUrl &u)
{
    m_url = u;
}

int QDeclarativeDebugFileReference::lineNumber() const
{
    return m_lineNumber;
}

void QDeclarativeDebugFileReference::setLineNumber(int l)
{
    m_lineNumber = l;
}

int QDeclarativeDebugFileReference::columnNumber() const
{
    return m_columnNumber;
}

void QDeclarativeDebugFileReference::setColumnNumber(int c)
{
    m_columnNumber = c;
}

QDeclarativeDebugPropertyReference::QDeclarativeDebugPropertyReference()
    : m_objectDebugId(-1), m_hasNotifySignal(false)
{
}

QDeclarativeDebugPropertyReference::QDeclarativeDebugPropertyReference(const QDeclarativeDebugPropertyReference &o)
    : m_objectDebugId(o.m_objectDebugId), m_name(o.m_name), m_value(o.m_value),
      m_valueTypeName(o.m_valueTypeName), m_binding(o.m_binding),
      m_hasNotifySignal(o.m_hasNotifySignal)
{
}

QDeclarativeDebugPropertyReference &QDeclarativeDebugPropertyReference::operator=(const QDeclarativeDebugPropertyReference &o)
{
    m_objectDebugId = o.m_objectDebugId; m_name = o.m_name; m_value = o.m_value;
    m_valueTypeName = o.m_valueTypeName; m_binding = o.m_binding;
    m_hasNotifySignal = o.m_hasNotifySignal;
    return *this;
}

int QDeclarativeDebugPropertyReference::objectDebugId() const
{
    return m_objectDebugId;
}

QString QDeclarativeDebugPropertyReference::name() const
{
    return m_name;
}

QString QDeclarativeDebugPropertyReference::valueTypeName() const
{
    return m_valueTypeName;
}

QVariant QDeclarativeDebugPropertyReference::value() const
{
    return m_value;
}

QString QDeclarativeDebugPropertyReference::binding() const
{
    return m_binding;
}

bool QDeclarativeDebugPropertyReference::hasNotifySignal() const
{
    return m_hasNotifySignal;
}

} // namespace QmlJsDebugClient
