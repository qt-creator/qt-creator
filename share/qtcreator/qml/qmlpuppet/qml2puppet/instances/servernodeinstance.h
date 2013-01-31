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

#ifndef SERVERNODEINSTANCE_H
#define SERVERNODEINSTANCE_H

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

    void paint(QPainter *painter);
    QImage renderImage() const;

    ServerNodeInstance parent() const;
    bool hasParent() const;

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
    QString instanceType(const QString &name) const;
    QStringList propertyNames() const;


    bool hasBindingForProperty(const QString &name, bool *hasChanged = 0) const;

    bool isValid() const;
    void makeInvalid();
    bool hasContent() const;
    bool isResizable() const;
    bool isMovable() const;
    bool isInPositioner() const;

    bool isSubclassOf(const QString &superTypeName) const;
    bool isRootNodeInstance() const;

    bool isWrappingThisObject(QObject *object) const;

    QVariant resetVariant(const QString &name) const;

    bool hasAnchor(const QString &name) const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    QPair<QString, ServerNodeInstance> anchor(const QString &name) const;

    int penWidth() const;

    static void registerQmlTypes();

    void doComponentComplete();

    QList<ServerNodeInstance> childItems() const;

    QString id() const;
    qint32 instanceId() const;

    QObject* testHandle() const;
    QSharedPointer<Internal::ObjectNodeInstance> internalInstance() const;

    QList<ServerNodeInstance> stateInstances() const;

private: // functions
    ServerNodeInstance(const QSharedPointer<Internal::ObjectNodeInstance> &abstractInstance);

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyDynamicVariant(const QString &name, const QString &typeName, const QVariant &value);

    void setPropertyBinding(const QString &name, const QString &expression);
    void setPropertyDynamicBinding(const QString &name, const QString &typeName, const QString &expression);

    void resetProperty(const QString &name);
    void refreshProperty(const QString &name);

    void activateState();
    void deactivateState();
    void refreshState();

    bool updateStateVariant(const ServerNodeInstance &target, const QString &propertyName, const QVariant &value);
    bool updateStateBinding(const ServerNodeInstance &target, const QString &propertyName, const QString &expression);
    bool resetStateProperty(const ServerNodeInstance &target, const QString &propertyName, const QVariant &resetValue);

    static ServerNodeInstance create(NodeInstanceServer *nodeInstanceServer, const InstanceContainer &instanceContainer, ComponentWrap componentWrap);

    void setDeleteHeldInstance(bool deleteInstance);
    void reparent(const ServerNodeInstance &oldParentInstance, const QString &oldParentProperty, const ServerNodeInstance &newParentInstance, const QString &newParentProperty);


    void setId(const QString &id);

    static QSharedPointer<Internal::ObjectNodeInstance> createInstance(QObject *objectToBeWrapped);

    void paintUpdate();

    static bool isSubclassOf(QObject *object, const QByteArray &superTypeName);

    void setNodeSource(const QString &source);


    QObject *internalObject() const; // should be not used outside of the nodeinstances!!!!
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QQuickItem *internalSGItem() const;
#endif

private: // variables
    QSharedPointer<Internal::ObjectNodeInstance> m_nodeInstance;
};

uint qHash(const ServerNodeInstance &instance);
bool operator==(const ServerNodeInstance &first, const ServerNodeInstance &second);
QDebug operator<<(QDebug debug, const ServerNodeInstance &instance);
}

Q_DECLARE_METATYPE(QmlDesigner::ServerNodeInstance)

#endif // SERVERNODEINSTANCE_H
