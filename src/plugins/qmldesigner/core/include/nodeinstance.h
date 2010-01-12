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

#ifndef NODEINSTANCE_H
#define NODEINSTANCE_H

#include "corelib_global.h"
#include <QSharedPointer>
#include <QHash>
#include <QRectF>
#include <propertymetainfo.h>
#include <qmlanchors.h>

class QPainter;
class QStyleOptionGraphicsItem;
class QmlContext;
class QGraphicsItem;
class QGraphicsTransform;

namespace QmlDesigner {

class ModelNode;
class NodeInstanceView;
class Preview;
class NodeMetaInfo;
class NodeState;
class WidgetQueryView;


namespace Internal {
    class ObjectNodeInstance;
    class QmlGraphicsItemNodeInstance;
}

class CORESHARED_EXPORT NodeInstance
{
    friend class CORESHARED_EXPORT QmlDesigner::WidgetQueryView;
    friend CORESHARED_EXPORT class Preview;
    friend CORESHARED_EXPORT class NodeInstanceView;
    friend class QHash<ModelNode, NodeInstance>;
    friend CORESHARED_EXPORT uint qHash(const NodeInstance &instance);
    friend CORESHARED_EXPORT bool operator==(const NodeInstance &first, const NodeInstance &second);
    friend CORESHARED_EXPORT class NodeMetaInfo;
    friend class QmlDesigner::Internal::QmlGraphicsItemNodeInstance;
    friend class QmlDesigner::Internal::ObjectNodeInstance;
public:
    NodeInstance();
    ~NodeInstance();
    NodeInstance(const NodeInstance &other);
    NodeInstance& operator=(const NodeInstance &other);

    void paint(QPainter *painter) const;

    NodeInstance parent() const;
    bool hasParent() const;
    ModelNode modelNode() const;
    void setModelNode(const ModelNode &node);

    bool isTopLevel() const;

    bool isQmlGraphicsItem() const;
    bool isGraphicsScene() const;
    bool isGraphicsView() const;
    bool isGraphicsWidget() const;
    bool isProxyWidget() const;
    bool isWidget() const;
    bool isQmlView() const;
    bool isGraphicsObject() const;
    bool isTransition() const;

    bool equalGraphicsItem(QGraphicsItem *item) const;

    QRectF boundingRect() const;
    QPointF position() const;
    QSizeF size() const;
    QTransform transform() const;
    QTransform customTransform() const;
    QTransform sceneTransform() const;
    double rotation() const;
    double scale() const;
    QList<QGraphicsTransform *> transformations() const;
    QPointF transformOriginPoint() const;
    double zValue() const;

    double opacity() const;
    QVariant property(const QString &name) const;
    QVariant defaultValue(const QString &name) const;

    bool isVisible() const;
    bool isValid() const;
    void makeInvalid();
    bool hasContent() const;

    const QObject *testHandle() const;

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyDynamicVariant(const QString &name, const QString &typeName, const QVariant &value);

    void setPropertyBinding(const QString &name, const QString &expression);
    void setPropertyDynamicBinding(const QString &name, const QString &typeName, const QString &expression);

    bool hasAnchor(const QString &name) const;
    bool isAnchoredBy() const;
    QPair<QString, NodeInstance> anchor(const QString &name) const;

    int penWidth() const;

private: // functions
    NodeInstance(const QSharedPointer<Internal::ObjectNodeInstance> &abstractInstance);

    static NodeInstance create(NodeInstanceView *nodeInstanceView, const ModelNode &node, QObject *objectToBeWrapped);
    static NodeInstance create(NodeInstanceView *nodeInstanceView, const NodeMetaInfo &metaInfo, QmlContext *context);

    void setDeleteHeldInstance(bool deleteInstance);
    void reparent(const NodeInstance &oldParentInstance, const QString &oldParentProperty, const NodeInstance &newParentInstance, const QString &newParentProperty);

    void resetProperty(const QString &name);

    void setId(const QString &id);

    static QSharedPointer<Internal::ObjectNodeInstance> createInstance(const NodeMetaInfo &metaInfo, QmlContext *context, QObject *objectToBeWrapped);
    QSharedPointer<Internal::QmlGraphicsItemNodeInstance> qmlGraphicsItemNodeInstance() const;

    void paintUpdate();

    void show();
    void hide();


    QObject *internalObject() const; // should be not used outside of the nodeinstances!!!!

private: // variables
    QSharedPointer<Internal::ObjectNodeInstance> m_nodeInstance;
};

CORESHARED_EXPORT uint qHash(const NodeInstance &instance);
CORESHARED_EXPORT bool operator==(const NodeInstance &first, const NodeInstance &second);
}

Q_DECLARE_METATYPE(QmlDesigner::NodeInstance);

#endif // NODEINSTANCE_H
