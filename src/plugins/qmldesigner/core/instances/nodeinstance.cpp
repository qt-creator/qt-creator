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

#include "nodeinstance.h"

#include "objectnodeinstance.h"
#include "metainfo.h"
#include "graphicswidgetnodeinstance.h"
#include "widgetnodeinstance.h"
#include "qmlgraphicsitemnodeinstance.h"
#include "graphicsscenenodeinstance.h"
#include "graphicsviewnodeinstance.h"
#include "nodeinstanceview.h"
#include "qmlviewnodeinstance.h"
#include "dummynodeinstance.h"
#include "componentnodeinstance.h"
#include "qmltransitionnodeinstance.h"
#include "qmlpropertychangesnodeinstance.h"
#include "qmlstatenodeinstance.h"
#include "nodeabstractproperty.h"
#include "variantproperty.h"

#include <invalidnodeinstanceexception.h>

#include <QHash>
#include <QSet>

#include <QtDeclarative/QmlEngine>

/*!
  \class QmlDesigner::NodeInstance
  \ingroup CoreInstance
  \brief NodeInstance is a common handle for the actual object representation of a ModelNode.

   NodeInstance abstracts away the differences e.g. in terms of position and size
   for QWidget, QGraphicsView, QLayout etc objects. Multiple NodeInstance objects can share
   the pointer to the same instance object. The actual instance will be deleted when
   the last NodeInstance object referencing to it is deleted. This can be disabled by
   setDeleteHeldInstance().

   \see QmlDesigner::NodeInstanceView
*/

namespace QmlDesigner {

/*!
\brief Constructor - creates a invalid NodeInstance


\see NodeInstanceView
*/
NodeInstance::NodeInstance()
{
}

/*!
\brief Destructor

*/
NodeInstance::~NodeInstance()
{
}

/*!
\brief Constructor - creates a valid NodeInstance

*/
NodeInstance::NodeInstance(const Internal::ObjectNodeInstance::Pointer &abstractInstance)
  : m_nodeInstance(abstractInstance)
{

}


NodeInstance::NodeInstance(const NodeInstance &other)
  : m_nodeInstance(other.m_nodeInstance)
{
}

NodeInstance &NodeInstance::operator=(const NodeInstance &other)
{
    m_nodeInstance = other.m_nodeInstance;
    return *this;
}

/*!
\brief Paints the NodeInstance with this painter.
\param painter used QPainter
*/
void NodeInstance::paint(QPainter *painter) const
{
    m_nodeInstance->paint(painter);
}

/*!
\brief Creates a new NodeInstace for this NodeMetaInfo

\param metaInfo MetaInfo for which a Instance should be created
\param context QmlContext which should be used
\returns Internal Pointer of a NodeInstance
\see NodeMetaInfo
*/
Internal::ObjectNodeInstance::Pointer NodeInstance::createInstance(const NodeMetaInfo &metaInfo,
                                                                     QmlContext *context, QObject *objectToBeWrapped)
{
    Internal::ObjectNodeInstance::Pointer instance;

    if (metaInfo.isSubclassOf("Qt/QGraphicsView", 4, 6))
        instance = Internal::GraphicsViewNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/QmlView", 4, 6))
        instance = Internal::QmlViewNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/QGraphicsWidget", 4, 6))
        instance = Internal::GraphicsWidgetNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/Item", 4, 6))
        instance = Internal::QmlGraphicsItemNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/QWidget", 4, 6))
        instance = Internal::WidgetNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/QGraphicsScene", 4, 6))
        instance = Internal::GraphicsSceneNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/Component", 4, 6))
        instance = Internal::ComponentNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/PropertyChanges", 4, 6))
        instance = Internal::QmlPropertyChangesNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/State", 4, 6))
        instance = Internal::QmlStateNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/Transition", 4, 6))
        instance = Internal::QmlTransitionNodeInstance::create(metaInfo, context, objectToBeWrapped);
    else if (metaInfo.isSubclassOf("Qt/QtObject", 4, 6))
        instance = Internal::ObjectNodeInstance::create(metaInfo, context, objectToBeWrapped);

    if (instance.isNull()) {
        instance = Internal::DummyNodeInstance::create(metaInfo, context);
    }

    return instance;
}



NodeInstance NodeInstance::create(NodeInstanceView *nodeInstanceView, const ModelNode &node, QObject *objectToBeWrapped)
{
    Q_ASSERT(node.isValid());
    Q_ASSERT(nodeInstanceView);

    // For the moment just use the root context of the engine
    // for all items. However, this is a hack ... ideally we should
    // rebuild the same context hierarchy as the qml compiler does

    QmlContext *context = nodeInstanceView->engine()->rootContext();

    NodeInstance instance(createInstance(node.metaInfo(), context, objectToBeWrapped));

    instance.m_nodeInstance->setModelNode(node);

    instance.m_nodeInstance->setNodeInstance(nodeInstanceView);

    instance.setId(node.id());

    foreach (const VariantProperty &property, node.variantProperties()) {
        if (property.isDynamic())
            instance.setPropertyDynamicVariant(property.name(), property.dynamicTypeName(), property.value());
        else
            instance.setPropertyVariant(property.name(), property.value());
    }

    return instance;
}

NodeInstance NodeInstance::create(NodeInstanceView *nodeInstanceView, const NodeMetaInfo &metaInfo, QmlContext *context)
{
    NodeInstance instance(createInstance(metaInfo, context, 0));
    instance.m_nodeInstance->setNodeInstance(nodeInstanceView);

    return instance;
}

/*!
\brief Returns the ModelNode of this NodeInstance.
\returns ModelNode of this NodeState
*/
ModelNode NodeInstance::modelNode() const
{
    if (m_nodeInstance.isNull())
        return ModelNode();

    return m_nodeInstance->modelNode();
}


/*!
\brief Changes the NodeState of the ModelNode of this NodeInstance.
    All properties are updated.
\param state NodeState of this NodeInstance
*/
void NodeInstance::setModelNode(const ModelNode &node)
{
    Q_ASSERT(node.isValid());
    if (m_nodeInstance->modelNode() == node)
        return;

    m_nodeInstance->setModelNode(node);
}

/*!
\brief Returns if the NodeInstance is a top level item.
\returns true if this NodeInstance is a top level item
*/
bool NodeInstance::isTopLevel() const
{
    return m_nodeInstance->isTopLevel();
}

void NodeInstance::reparent(const NodeInstance &oldParentInstance, const QString &oldParentProperty, const NodeInstance &newParentInstance, const QString &newParentProperty)
{
    m_nodeInstance->reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);
}


/*!
\brief Returns the parent NodeInstance of this NodeInstance.

    If there is not parent than the parent is invalid.

\returns Parent NodeInstance.
*/
NodeInstance NodeInstance::parent() const
{
    return m_nodeInstance->nodeInstanceView()->instanceForObject(m_nodeInstance->parent());
}

bool NodeInstance::hasParent() const
{
    return m_nodeInstance->object()->parent();
}

/*!
\brief Returns if the NodeInstance is a QmlGraphicsItem.
\returns true if this NodeInstance is a QmlGraphicsItem
*/
bool NodeInstance::isQmlGraphicsItem() const
{
    return m_nodeInstance->isQmlGraphicsItem();
}

/*!
\brief Returns if the NodeInstance is a QGraphicsScene.
\returns true if this NodeInstance is a QGraphicsScene
*/
bool NodeInstance::isGraphicsScene() const
{
    return m_nodeInstance->isGraphicsScene();
}

/*!
\brief Returns if the NodeInstance is a QGraphicsView.
\returns true if this NodeInstance is a QGraphicsView
*/
bool NodeInstance::isGraphicsView() const
{
    return m_nodeInstance->isGraphicsView();
}

/*!
\brief Returns if the NodeInstance is a QGraphicsWidget.
\returns true if this NodeInstance is a QGraphicsWidget
*/
bool NodeInstance::isGraphicsWidget() const
{
    return m_nodeInstance->isGraphicsWidget();
}

/*!
\brief Returns if the NodeInstance is a QGraphicsProxyWidget.
\returns true if this NodeInstance is a QGraphicsProxyWidget
*/
bool NodeInstance::isProxyWidget() const
{
    return m_nodeInstance->isProxyWidget();
}

/*!
\brief Returns if the NodeInstance is a QWidget.
\returns true if this NodeInstance is a QWidget
*/
bool NodeInstance::isWidget() const
{
    return m_nodeInstance->isWidget();
}

/*!
\brief Returns if the NodeInstance is a QmlView.
\returns true if this NodeInstance is a QmlView
*/
bool NodeInstance::isQmlView() const
{
    return m_nodeInstance->isQmlView();
}

bool NodeInstance::isGraphicsObject() const
{
    return m_nodeInstance->isGraphicsObject();
}

bool NodeInstance::isTransition() const
{
    return m_nodeInstance->isTransition();
}

/*!
\brief Returns if the NodeInstance is a QGraphicsItem.
\returns true if this NodeInstance is a QGraphicsItem
*/
bool NodeInstance::equalGraphicsItem(QGraphicsItem *item) const
{
    return m_nodeInstance->equalGraphicsItem(item);
}

/*!
\brief Returns the bounding rect of the NodeInstance.
\returns QRectF of the NodeInstance
*/
QRectF NodeInstance::boundingRect() const
{
    QRectF boundingRect(m_nodeInstance->boundingRect());

//
//    if (modelNode().isValid()) { // TODO implement recursiv stuff
//        if (qFuzzyIsNull(boundingRect.width()))
//            boundingRect.setWidth(nodeState().property("width").value().toDouble());
//
//        if (qFuzzyIsNull(boundingRect.height()))
//            boundingRect.setHeight(nodeState().property("height").value().toDouble());
//    }

    return boundingRect;
}

void NodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    m_nodeInstance->setPropertyVariant(name, value);
}

void NodeInstance::setPropertyDynamicVariant(const QString &name, const QString &typeName, const QVariant &value)
{
    m_nodeInstance->createDynamicProperty(name, typeName);
    m_nodeInstance->setPropertyVariant(name, value);
}

void NodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    m_nodeInstance->setPropertyBinding(name, expression);
}

void NodeInstance::setPropertyDynamicBinding(const QString &name, const QString &typeName, const QString &expression)
{
    m_nodeInstance->createDynamicProperty(name, typeName);
    m_nodeInstance->setPropertyBinding(name, expression);
}

void NodeInstance::resetProperty(const QString &name)
{
    m_nodeInstance->resetProperty(name);
}

void NodeInstance::setId(const QString &id)
{
    m_nodeInstance->setId(id);
}

/*!
\brief Returns the property value of the property of this NodeInstance.
\returns QVariant value
*/
QVariant NodeInstance::property(const QString &name) const
{
    return m_nodeInstance->property(name);
}

/*!
\brief Returns the property default value of the property of this NodeInstance.
\returns QVariant default value which is the reset value to
*/
QVariant NodeInstance::defaultValue(const QString &name) const
{
    return m_nodeInstance->resetValue(name);
}

/*!
\brief Returns if the NodeInstance is visible.
\returns true if the NodeInstance is visible
*/
bool NodeInstance::isVisible() const
{
    return m_nodeInstance->isVisible();
}

void NodeInstance::show()
{
    m_nodeInstance->setVisible(true);
}
void NodeInstance::hide()
{
    m_nodeInstance->setVisible(false);
}

/*!
\brief Returns if the NodeInstance is valid.
\returns true if the NodeInstance is valid
*/
bool NodeInstance::isValid() const
{
    return m_nodeInstance && internalObject();
}

void NodeInstance::makeInvalid()
{
    if (m_nodeInstance)
        m_nodeInstance->destroy();
    m_nodeInstance.clear();
}

bool NodeInstance::hasContent() const
{
    return m_nodeInstance->hasContent();
}

bool NodeInstance::hasAnchor(const QString &name) const
{
    return m_nodeInstance->hasAnchor(name);
}

int NodeInstance::penWidth() const
{
    return m_nodeInstance->penWidth();
}

bool NodeInstance::isAnchoredBy() const
{
    return m_nodeInstance->isAnchoredBy();
}

QPair<QString, NodeInstance> NodeInstance::anchor(const QString &name) const
{
    return m_nodeInstance->anchor(name);
}

uint qHash(const NodeInstance &instance)
{
    return ::qHash(instance.m_nodeInstance.data());
}

bool operator==(const NodeInstance &first, const NodeInstance &second)
{
    return first.m_nodeInstance.data() == second.m_nodeInstance.data();
}

const QObject *NodeInstance::testHandle() const
{
    return internalObject();
}

/*!
\brief Returns the position in parent coordiantes.
\returns QPointF of the position of the instance.
*/
QPointF NodeInstance::position() const
{
    return m_nodeInstance->position();
}

/*!
\brief Returns the size in local coordiantes.
\returns QSizeF of the size of the instance.
*/
QSizeF NodeInstance::size() const
{
    QSizeF instanceSize = m_nodeInstance->size();

//    if (nodeState().isValid()) {
//        if (qFuzzyIsNull(instanceSize.width()))
//            instanceSize.setWidth(nodeState().property("width").value().toDouble());
//
//        if (qFuzzyIsNull(instanceSize.height()))
//            instanceSize.setHeight(nodeState().property("height").value().toDouble());
//    }
    return instanceSize;
}

QTransform NodeInstance::transform() const
{
    return m_nodeInstance->transform();
}

/*!
\brief Returns the transform matrix of the instance.
\returns QTransform of the instance.
*/
QTransform NodeInstance::customTransform() const
{
    return m_nodeInstance->customTransform();
}

QTransform NodeInstance::sceneTransform() const
{
    return m_nodeInstance->sceneTransform();
}

double NodeInstance::rotation() const
{
    return m_nodeInstance->rotation();
}

double NodeInstance::scale() const
{
    return m_nodeInstance->scale();
}

QList<QGraphicsTransform *> NodeInstance::transformations() const
{
    return m_nodeInstance->transformations();
}

QPointF NodeInstance::transformOriginPoint() const
{
    return m_nodeInstance->transformOriginPoint();
}

double NodeInstance::zValue() const
{
    return m_nodeInstance->zValue();
}

/*!
\brief Returns the opacity of the instance.
\returns 0.0 mean transparent and 1.0 opaque.
*/
double NodeInstance::opacity() const
{
    return m_nodeInstance->opacity();
}


void NodeInstance::setDeleteHeldInstance(bool deleteInstance)
{
    m_nodeInstance->setDeleteHeldInstance(deleteInstance);
}


void NodeInstance::paintUpdate()
{
    m_nodeInstance->paintUpdate();
}


Internal::QmlGraphicsItemNodeInstance::Pointer NodeInstance::qmlGraphicsItemNodeInstance() const
{
    return m_nodeInstance.dynamicCast<Internal::QmlGraphicsItemNodeInstance>();
}

QObject *NodeInstance::internalObject() const
{
    if (m_nodeInstance.isNull())
        return 0;

    return m_nodeInstance->object();
}

}
