/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ABSTRACTNODEINSTANCE_H
#define ABSTRACTNODEINSTANCE_H

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QWeakPointer>
#include "nodeinstanceserver.h"
#include "nodeinstancemetaobject.h"
#include "nodeinstancesignalspy.h"

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QDeclarativeContext;
class QDeclarativeEngine;
class QDeclarativeProperty;
class QDeclarativeContext;
class QDeclarativeBinding;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServer;


namespace Internal {

class QmlGraphicsItemNodeInstance;
class GraphicsWidgetNodeInstance;
class GraphicsViewNodeInstance;
class GraphicsSceneNodeInstance;
class ProxyWidgetNodeInstance;
class WidgetNodeInstance;

class ObjectNodeInstance
{
public:
    typedef QSharedPointer<ObjectNodeInstance> Pointer;
    typedef QWeakPointer<ObjectNodeInstance> WeakPointer;
    ObjectNodeInstance(QObject *object);

    virtual ~ObjectNodeInstance();
    void destroy();
    //void setModelNode(const ModelNode &node);

    static Pointer create(QObject *objectToBeWrapped);
    static QObject* createObject(const QString &typeName, int majorNumber, int minorNumber, const QString &componentPath, QDeclarativeContext *context);

    void setInstanceId(qint32 id);
    qint32 instanceId() const;

    NodeInstanceServer *nodeInstanceServer() const;
    void setNodeInstanceServer(NodeInstanceServer *server);
    virtual void initializePropertyWatcher(const Pointer &objectNodeInstance);
    virtual void paint(QPainter *painter);
    virtual QImage renderImage() const;

    virtual QObject *parent() const;

    Pointer parentInstance() const;

    virtual void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty);

    void setId(const QString &id);
    QString id() const;

    virtual bool isQmlGraphicsItem() const;
    virtual bool isGraphicsObject() const;
    virtual bool isTransition() const;
    virtual bool isPositioner() const;


    virtual bool equalGraphicsItem(QGraphicsItem *item) const;

    virtual QRectF boundingRect() const;

    virtual QPointF position() const;
    virtual QSizeF size() const;
    virtual QTransform transform() const;
    virtual QTransform customTransform() const;
    virtual QTransform sceneTransform() const;
    virtual double opacity() const;

    virtual int penWidth() const;

    virtual bool hasAnchor(const QString &name) const;
    virtual QPair<QString, ServerNodeInstance> anchor(const QString &name) const;
    virtual bool isAnchoredBySibling() const;
    virtual bool isAnchoredByChildren() const;

    virtual double rotation() const;
    virtual double scale() const;
    virtual QList<QGraphicsTransform *> transformations() const;
    virtual QPointF transformOriginPoint() const;
    virtual double zValue() const;

    virtual void setPropertyVariant(const QString &name, const QVariant &value);
    virtual void setPropertyBinding(const QString &name, const QString &expression);
    virtual QVariant property(const QString &name) const;
    virtual void resetProperty(const QString &name);
    virtual void refreshProperty(const QString &name);
    virtual QString instanceType(const QString &name) const;
    QStringList propertyNames() const;

    virtual QList<ServerNodeInstance> childItems() const;

    void createDynamicProperty(const QString &name, const QString &typeName);
    void setDeleteHeldInstance(bool deleteInstance);
    bool deleteHeldInstance() const;

    virtual void updateAnchors();
    virtual void paintUpdate();

    virtual void activateState();
    virtual void deactivateState();

    void populateResetValueHash();
    QVariant resetValue(const QString &propertyName) const;

    QObject *object() const;

    virtual bool hasContent() const;
    virtual bool isResizable() const;
    virtual bool isMovable() const;
    bool isInPositioner() const;
    void setInPositioner(bool isInPositioner);

    bool hasBindingForProperty(const QString &name, bool *hasChanged = 0) const;

    QDeclarativeContext *context() const;
    QDeclarativeEngine *engine() const;

    virtual bool updateStateVariant(const ObjectNodeInstance::Pointer &target, const QString &propertyName, const QVariant &value);
    virtual bool updateStateBinding(const ObjectNodeInstance::Pointer &target, const QString &propertyName, const QString &expression);
    virtual bool resetStateProperty(const ObjectNodeInstance::Pointer &target, const QString &propertyName, const QVariant &resetValue);


    bool isValid() const;
    bool isRootNodeInstance() const;

    virtual void doComponentComplete();


protected:
    void doResetProperty(const QString &propertyName);
    void removeFromOldProperty(QObject *object, QObject *oldParent, const QString &oldParentProperty);
    void addToNewProperty(QObject *object, QObject *newParent, const QString &newParentProperty);
    void deleteObjectsInList(const QDeclarativeProperty &metaProperty);

private:
    static void refreshBindings(QDeclarativeContext *context);

    QHash<QString, QVariant> m_resetValueHash;
    QHash<QString, ServerNodeInstance> m_modelAbstractPropertyHash;
    mutable QHash<QString, bool> m_hasBindingHash;
    qint32 m_instanceId;
    QString m_id;

    QWeakPointer<NodeInstanceServer> m_nodeInstanceServer;
    QString m_parentProperty;
    bool m_deleteHeldInstance;
    QWeakPointer<QObject> m_object;
    NodeInstanceMetaObject *m_metaObject;
    NodeInstanceSignalSpy m_signalSpy;
    bool m_isInPositioner;
};


} // namespace Internal
} // namespace QmlDesigner

#endif // ABSTRACTNODEINSTANCE_H
