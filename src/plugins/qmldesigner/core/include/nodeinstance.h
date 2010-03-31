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

#ifndef NODEINSTANCE_H
#define NODEINSTANCE_H

#include "corelib_global.h"
#include <QSharedPointer>
#include <QHash>
#include <QRectF>
#include <propertymetainfo.h>
#include <qmlanchors.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
class QDeclarativeContext;
class QGraphicsItem;
class QGraphicsTransform;
QT_END_NAMESPACE

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
    class QmlPropertyChangesNodeInstance;
    class QmlStateNodeInstance;
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
    friend class QmlDesigner::Internal::QmlPropertyChangesNodeInstance;
    friend class QmlDesigner::Internal::QmlStateNodeInstance;

public:
    NodeInstance();
    ~NodeInstance();
    NodeInstance(const NodeInstance &other);
    NodeInstance& operator=(const NodeInstance &other);

    void paint(QPainter *painter) const;

    NodeInstance parent() const;
    bool hasParent() const;
    ModelNode modelNode() const;


    bool isTopLevel() const;

    bool isQmlGraphicsItem() const;
    bool isGraphicsScene() const;
    bool isGraphicsView() const;
    bool isGraphicsWidget() const;
    bool isProxyWidget() const;
    bool isWidget() const;
    bool isQDeclarativeView() const;
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

    bool isValid() const;
    void makeInvalid();
    bool hasContent() const;

    bool isWrappingThisObject(QObject *object) const;

    QVariant resetVariant(const QString &name) const;

    bool hasAnchor(const QString &name) const;
    bool isAnchoredBy() const;
    QPair<QString, NodeInstance> anchor(const QString &name) const;

    int penWidth() const;

    static void registerDeclarativeTypes();

#ifdef QTCREATOR_TEST
    QObject* testHandle() const;
#endif
private: // functions
    NodeInstance(const QSharedPointer<Internal::ObjectNodeInstance> &abstractInstance);

    void setModelNode(const ModelNode &node);

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyDynamicVariant(const QString &name, const QString &typeName, const QVariant &value);

    void setPropertyBinding(const QString &name, const QString &expression);
    void setPropertyDynamicBinding(const QString &name, const QString &typeName, const QString &expression);

    void resetProperty(const QString &name);
    void refreshProperty(const QString &name);

    void activateState();
    void deactivateState();
    void refreshState();

    bool updateStateVariant(const NodeInstance &target, const QString &propertyName, const QVariant &value);
    bool updateStateBinding(const NodeInstance &target, const QString &propertyName, const QString &expression);
    bool resetStateProperty(const NodeInstance &target, const QString &propertyName, const QVariant &resetValue);

    static NodeInstance create(NodeInstanceView *nodeInstanceView, const ModelNode &node, QObject *objectToBeWrapped);
    static NodeInstance create(NodeInstanceView *nodeInstanceView, const NodeMetaInfo &metaInfo, QDeclarativeContext *context);

    void setDeleteHeldInstance(bool deleteInstance);
    void reparent(const NodeInstance &oldParentInstance, const QString &oldParentProperty, const NodeInstance &newParentInstance, const QString &newParentProperty);


    void setId(const QString &id);

    static QSharedPointer<Internal::ObjectNodeInstance> createInstance(const NodeMetaInfo &metaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped);
    QSharedPointer<Internal::QmlGraphicsItemNodeInstance> qmlGraphicsItemNodeInstance() const;

    void paintUpdate();


    QObject *internalObject() const; // should be not used outside of the nodeinstances!!!!

private: // variables
    QSharedPointer<Internal::ObjectNodeInstance> m_nodeInstance;
};

CORESHARED_EXPORT uint qHash(const NodeInstance &instance);
CORESHARED_EXPORT bool operator==(const NodeInstance &first, const NodeInstance &second);
}

Q_DECLARE_METATYPE(QmlDesigner::NodeInstance);

#endif // NODEINSTANCE_H
