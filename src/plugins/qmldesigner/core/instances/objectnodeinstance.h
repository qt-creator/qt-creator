/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "modelnode.h"
#include <QSharedPointer>
#include <QScopedPointer>
#include <QWeakPointer>
#include <propertymetainfo.h>
#include <nodeinstanceview.h>
#include "nodeinstancemetaobject.h"
#include "nodeinstancesignalspy.h"

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QmlContext;
class QmlMetaProperty;
class QmlContext;
class QmlBinding;
QT_END_NAMESPACE

namespace QmlDesigner {


namespace Internal {

class QmlGraphicsItemNodeInstance;
class GraphicsWidgetNodeInstance;
class GraphicsViewNodeInstance;
class GraphicsSceneNodeInstance;
class ProxyWidgetNodeInstance;
class WidgetNodeInstance;
class QmlViewNodeInstance;

class ChildrenChangeEventFilter : public QObject
{
    Q_OBJECT
public:
    ChildrenChangeEventFilter(QObject *parent);


signals:
    void childrenChanged(QObject *object);

protected:
    bool eventFilter(QObject *object, QEvent *event);

};

class ObjectNodeInstance
{
public:
    typedef QSharedPointer<ObjectNodeInstance> Pointer;
    typedef QWeakPointer<ObjectNodeInstance> WeakPointer;
    ObjectNodeInstance(QObject *object);

    virtual ~ObjectNodeInstance();
    void destroy();
    //void setModelNode(const ModelNode &node);

    static Pointer create(const NodeMetaInfo &metaInfo, QmlContext *context, QObject *objectToBeWrapped);

    ModelNode modelNode() const;
    void setModelNode(const ModelNode &node);

    NodeInstanceView *nodeInstanceView() const;
    void setNodeInstanceView(NodeInstanceView *view);
    void initializePropertyWatcher(const Pointer &objectNodeInstance);
    virtual void paint(QPainter *painter) const;

    virtual bool isTopLevel() const;

    virtual QObject *parent() const;

    void reparent(const NodeInstance &oldParentInstance, const QString &oldParentProperty, const NodeInstance &newParentInstance, const QString &newParentProperty);

    void setId(const QString &id);

    virtual bool isQmlGraphicsItem() const;
    virtual bool isGraphicsScene() const;
    virtual bool isGraphicsView() const;
    virtual bool isGraphicsWidget() const;
    virtual bool isProxyWidget() const;
    virtual bool isWidget() const;
    virtual bool isQmlView() const;
    virtual bool isGraphicsObject() const;
    virtual bool isTransition() const;

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
    virtual QPair<QString, NodeInstance> anchor(const QString &name) const;
    virtual bool isAnchoredBy() const;

    virtual double rotation() const;
    virtual double scale() const;
    virtual QList<QGraphicsTransform *> transformations() const;
    virtual QPointF transformOriginPoint() const;
    virtual double zValue() const;

    virtual void setPropertyVariant(const QString &name, const QVariant &value);
    virtual void setPropertyBinding(const QString &name, const QString &expression);
    virtual QVariant property(const QString &name) const;
    virtual void resetProperty(const QString &name);
    virtual bool isVisible() const;
    virtual void setVisible(bool isVisible);

    void createDynamicProperty(const QString &name, const QString &typeName);
    void setDeleteHeldInstance(bool deleteInstance);
    bool deleteHeldInstance() const;

    virtual void updateAnchors();
    virtual void paintUpdate();

    virtual void activateState();
    virtual void deactivateState();
    virtual void refreshState();

    void populateResetValueHash();
    QVariant resetValue(const QString &propertyName) const;

    QObject *object() const;

    virtual bool hasContent() const;

    QmlContext *context() const;

protected:
    static QObject* createObject(const NodeMetaInfo &metaInfo, QmlContext *context);

    void resetProperty(QObject *object, const QString &propertyName);
    NodeInstance instanceForNode(const ModelNode &node, const QString &fullname);

    void removeFromOldProperty(QObject *object, QObject *oldParent, const QString &oldParentProperty);
    void addToNewProperty(QObject *object, QObject *newParent, const QString &newParentProperty);
    void deleteObjectsInList(const QmlMetaProperty &metaProperty);

private:
    static void refreshBindings(QmlContext *context);

    QHash<QString, QVariant> m_resetValueHash;
    QHash<QString, NodeInstance> m_modelAbstractPropertyHash;
    ModelNode m_modelNode;
    QString m_id;

    QWeakPointer<NodeInstanceView> m_nodeInstanceView;
    bool m_deleteHeldInstance;
    QWeakPointer<QObject> m_object;
    NodeInstanceMetaObject *m_metaObject;
    NodeInstanceSignalSpy m_signalSpy;

};


} // namespace Internal
} // namespace QmlDesigner

#endif // ABSTRACTNODEINSTANCE_H
