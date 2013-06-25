/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OBJECTNODEINSTANCE_H
#define OBJECTNODEINSTANCE_H

#include "nodeinstanceserver.h"
#include "nodeinstancemetaobject.h"
#include "nodeinstancesignalspy.h"

#include <QPainter>
#include <QSharedPointer>
#include <QWeakPointer>

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QQmlContext;
class QQmlEngine;
class QQmlProperty;
class QQmlAbstractBinding;
class QQuickItem;
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

    virtual ~ObjectNodeInstance();
    void destroy();
    //void setModelNode(const ModelNode &node);

    static Pointer create(QObject *objectToBeWrapped);
    static QObject *createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context);
    static QObject *createCustomParserObject(const QString &nodeSource, const QStringList &imports, QQmlContext *context);
    static QObject *createComponent(const QString &componentPath, QQmlContext *context);
    static QObject *createComponentWrap(const QString &nodeSource, const QStringList &imports, QQmlContext *context);

    void setInstanceId(qint32 id);
    qint32 instanceId() const;

    NodeInstanceServer *nodeInstanceServer() const;
    void setNodeInstanceServer(NodeInstanceServer *server);
    virtual void initializePropertyWatcher(const Pointer &objectNodeInstance);
    virtual void initialize(const Pointer &objectNodeInstance);
    virtual QImage renderImage() const;
    virtual QImage renderPreviewImage(const QSize &previewImageSize) const;

    virtual QObject *parent() const;

    Pointer parentInstance() const;

    virtual void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty);

    virtual void setId(const QString &id);
    virtual QString id() const;

    virtual bool isTransition() const;
    virtual bool isPositioner() const;
    virtual bool isQuickItem() const;
    virtual bool isQuickWindow() const;
    virtual bool isGraphical() const;
    virtual bool isLayoutable() const;

    virtual bool equalGraphicsItem(QGraphicsItem *item) const;

    virtual QRectF boundingRect() const;
    virtual QRectF contentItemBoundingBox() const;

    virtual QPointF position() const;
    virtual QSizeF size() const;
    virtual QTransform transform() const;
    virtual QTransform contentTransform() const;
    virtual QTransform customTransform() const;
    virtual QTransform contentItemTransform() const;
    virtual QTransform sceneTransform() const;
    virtual double opacity() const;

    virtual int penWidth() const;

    virtual bool hasAnchor(const PropertyName &name) const;
    virtual QPair<PropertyName, ServerNodeInstance> anchor(const PropertyName &name) const;
    virtual bool isAnchoredBySibling() const;
    virtual bool isAnchoredByChildren() const;

    virtual double rotation() const;
    virtual double scale() const;
    virtual QList<QGraphicsTransform *> transformations() const;
    virtual QPointF transformOriginPoint() const;
    virtual double zValue() const;

    virtual void setPropertyVariant(const PropertyName &name, const QVariant &value);
    virtual void setPropertyBinding(const PropertyName &name, const QString &expression);
    virtual QVariant property(const PropertyName &name) const;
    virtual void resetProperty(const PropertyName &name);
    virtual void refreshProperty(const PropertyName &name);
    virtual QString instanceType(const PropertyName &name) const;
    PropertyNameList propertyNames() const;

    virtual QList<ServerNodeInstance> childItems() const;

    void createDynamicProperty(const QString &PropertyName, const QString &typeName);
    void setDeleteHeldInstance(bool deleteInstance);
    bool deleteHeldInstance() const;

    virtual void updateAnchors();
    virtual void paintUpdate();

    virtual void activateState();
    virtual void deactivateState();

    void populateResetHashes();
    bool hasValidResetBinding(const PropertyName &propertyName) const;
    QQmlAbstractBinding *resetBinding(const PropertyName &propertyName) const;
    QVariant resetValue(const PropertyName &propertyName) const;
    void setResetValue(const PropertyName &propertyName, const QVariant &value);

    QObject *object() const;
    virtual QQuickItem *contentItem() const;

    virtual bool hasContent() const;
    virtual bool isResizable() const;
    virtual bool isMovable() const;
    bool isInLayoutable() const;
    void setInLayoutable(bool isInLayoutable);
    virtual void refreshLayoutable();

    bool hasBindingForProperty(const PropertyName &name, bool *hasChanged = 0) const;

    QQmlContext *context() const;
    QQmlEngine *engine() const;

    virtual bool updateStateVariant(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant &value);
    virtual bool updateStateBinding(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QString &expression);
    virtual bool resetStateProperty(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant &resetValue);


    bool isValid() const;
    bool isRootNodeInstance() const;

    virtual void doComponentComplete();

    virtual QList<ServerNodeInstance> stateInstances() const;

    virtual void setNodeSource(const QString &source);

    static QVariant fixResourcePaths(const QVariant &value);

    virtual void updateAllDirtyNodesRecursive();

protected:
    explicit ObjectNodeInstance(QObject *object);
    void doResetProperty(const PropertyName &propertyName);
    void removeFromOldProperty(QObject *object, QObject *oldParent, const PropertyName &oldParentProperty);
    void addToNewProperty(QObject *object, QObject *newParent, const PropertyName &newParentProperty);
    void deleteObjectsInList(const QQmlProperty &metaProperty);
    QVariant convertSpecialCharacter(const QVariant& value) const;
    static QObject *parentObject(QObject *object);
    static void doComponentCompleteRecursive(QObject *object, NodeInstanceServer *nodeInstanceServer);

private:
    QHash<PropertyName, QVariant> m_resetValueHash;
    QHash<PropertyName, QWeakPointer<QQmlAbstractBinding> > m_resetBindingHash;
    QHash<PropertyName, ServerNodeInstance> m_modelAbstractPropertyHash;
    mutable QHash<PropertyName, bool> m_hasBindingHash;
    QString m_id;

    QPointer<NodeInstanceServer> m_nodeInstanceServer;
    PropertyName m_parentProperty;

    QPointer<QObject> m_object;
    NodeInstanceMetaObject *m_metaObject;
    NodeInstanceSignalSpy m_signalSpy;
    qint32 m_instanceId;
    bool m_deleteHeldInstance;
    bool m_isInLayoutable;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // OBJECTNODEINSTANCE_H
