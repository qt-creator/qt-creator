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

#pragma once

#include <QSharedPointer>
#include <QHash>
#include <QRectF>

#include <nodeinstanceserverinterface.h>
#include <propertyvaluecontainer.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
class QQmlContext;
class QGraphicsItem;
class QGraphicsTransform;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
class QQuickItem;
#endif
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServer;
class Qt4NodeInstanceServer;
class Qt4PreviewNodeInstanceServer;
class Qt5NodeInstanceServer;
class Qt5PreviewNodeInstanceServer;
class Qt5TestNodeInstanceServer;
class InstanceContainer;

namespace Internal {
    class ObjectNodeInstance;
    class QmlGraphicsItemNodeInstance;
    class QmlPropertyChangesNodeInstance;
    class GraphicsObjectNodeInstance;
    class QmlStateNodeInstance;
    class QuickItemNodeInstance;
}

class ServerNodeInstance
{
    friend class NodeInstanceServer;
    friend class Qt4NodeInstanceServer;
    friend class Qt4PreviewNodeInstanceServer;
    friend class Qt5NodeInstanceServer;
    friend class Qt5PreviewNodeInstanceServer;
    friend class Qt5TestNodeInstanceServer;
    friend class QHash<qint32, ServerNodeInstance>;
    friend uint qHash(const ServerNodeInstance &instance);
    friend bool operator==(const ServerNodeInstance &first, const ServerNodeInstance &second);
    friend QDebug operator<<(QDebug debug, const ServerNodeInstance &instance);
    friend class NodeMetaInfo;
    friend class QmlDesigner::Internal::QmlGraphicsItemNodeInstance;
    friend class QmlDesigner::Internal::QuickItemNodeInstance;
    friend class QmlDesigner::Internal::GraphicsObjectNodeInstance;
    friend class QmlDesigner::Internal::ObjectNodeInstance;
    friend class QmlDesigner::Internal::QmlPropertyChangesNodeInstance;
    friend class QmlDesigner::Internal::QmlStateNodeInstance;

public:
    enum ComponentWrap {
        WrapAsComponent,
        DoNotWrapAsComponent
    };

    ServerNodeInstance();
    ~ServerNodeInstance();
    ServerNodeInstance(const ServerNodeInstance &other);
    ServerNodeInstance& operator=(const ServerNodeInstance &other);

    QImage renderImage() const;
    QImage renderPreviewImage(const QSize &previewImageSize) const;

    ServerNodeInstance parent() const;
    bool hasParent() const;

    bool equalGraphicsItem(QGraphicsItem *item) const;

    QRectF boundingRect() const;
    QRectF contentItemBoundingRect() const;
    QPointF position() const;
    QSizeF size() const;
    QTransform transform() const;
    QTransform customTransform() const;
    QTransform sceneTransform() const;
    QTransform contentTransform() const;
    QTransform contentItemTransform() const;
    double rotation() const;
    double scale() const;
    QList<QGraphicsTransform *> transformations() const;
    QPointF transformOriginPoint() const;
    double zValue() const;

    double opacity() const;
    QVariant property(const PropertyName &name) const;
    QVariant defaultValue(const PropertyName &name) const;
    QString instanceType(const PropertyName &name) const;
    PropertyNameList propertyNames() const;


    bool hasBindingForProperty(const PropertyName &name, bool *hasChanged = 0) const;

    bool isValid() const;
    void makeInvalid();
    bool hasContent() const;
    bool isResizable() const;
    bool isMovable() const;
    bool isInLayoutable() const;

    bool isSubclassOf(const QString &superTypeName) const;
    bool isRootNodeInstance() const;

    bool isWrappingThisObject(QObject *object) const;

    QVariant resetVariant(const PropertyName &name) const;

    bool hasAnchor(const PropertyName &name) const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    QPair<PropertyName, ServerNodeInstance> anchor(const PropertyName &name) const;

    int penWidth() const;

    static void registerQmlTypes();

    void doComponentComplete();

    QList<ServerNodeInstance> childItems() const;

    QQuickItem *rootQuickItem() const;
    QList<QQuickItem *> allItemsRecursive() const;

    QString id() const;
    qint32 instanceId() const;

    QSharedPointer<Internal::ObjectNodeInstance> internalInstance() const;

    QList<ServerNodeInstance> stateInstances() const;

private: // functions
    ServerNodeInstance(const QSharedPointer<Internal::ObjectNodeInstance> &abstractInstance);

    void setPropertyVariant(const PropertyName &name, const QVariant &value);

    void setPropertyBinding(const PropertyName &name, const QString &expression);

    void resetProperty(const PropertyName &name);
    void refreshProperty(const PropertyName &name);

    void activateState();
    void deactivateState();
    void refreshState();

    bool updateStateVariant(const ServerNodeInstance &target, const PropertyName &propertyName, const QVariant &value);
    bool updateStateBinding(const ServerNodeInstance &target, const PropertyName &propertyName, const QString &expression);
    bool resetStateProperty(const ServerNodeInstance &target, const PropertyName &propertyName, const QVariant &resetValue);

    static ServerNodeInstance create(NodeInstanceServer *nodeInstanceServer, const InstanceContainer &instanceContainer, ComponentWrap componentWrap);

    void setDeleteHeldInstance(bool deleteInstance);
    void reparent(const ServerNodeInstance &oldParentInstance, const PropertyName &oldParentProperty, const ServerNodeInstance &newParentInstance, const PropertyName &newParentProperty);


    void setId(const QString &id);

    static QSharedPointer<Internal::ObjectNodeInstance> createInstance(QObject *objectToBeWrapped);

    void paintUpdate();

    static bool isSubclassOf(QObject *object, const QByteArray &superTypeName);

    void setNodeSource(const QString &source);

    bool holdsGraphical() const;

    void updateDirtyNodeRecursive();

    QObject *internalObject() const; // should be not used outside of the nodeinstances!!!!

private: // variables
    QSharedPointer<Internal::ObjectNodeInstance> m_nodeInstance;
};

uint qHash(const ServerNodeInstance &instance);
bool operator ==(const ServerNodeInstance &first, const ServerNodeInstance &second);
bool operator <(const ServerNodeInstance &first, const ServerNodeInstance &second);
QDebug operator <<(QDebug debug, const ServerNodeInstance &instance);
}

Q_DECLARE_METATYPE(QmlDesigner::ServerNodeInstance)
