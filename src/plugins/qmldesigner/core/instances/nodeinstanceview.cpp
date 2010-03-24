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

#include "nodeinstanceview.h"

#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <private/qdeclarativeengine_p.h>

#include <QtDebug>
#include <QUrl>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsObject>

#include <model.h>
#include <modelnode.h>
#include <propertymetainfo.h>
#include <metainfo.h>
#include <nodeinstance.h>

#include <typeinfo>
#include <iwidgetplugin.h>

#include "abstractproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"

#include "objectnodeinstance.h"

#include "qmlmodelview.h"

enum {
    debug = false
};

/*!
\defgroup CoreInstance
*/
/*!
\class QmlDesigner::NodeInstanceView
\ingroup CoreInstance
\brief Central class to create and manage instances of a ModelNode.

This view is used to instance the ModelNodes. Many AbstractViews hold a
NodeInstanceView to get values from tghe NodeInstances back.
For this purpose this view can be rendered offscreen.

\see NodeInstance ModelNode
*/

namespace QmlDesigner {

/*! \brief Constructor

  The class will be rendered offscreen if not set otherwise.

\param Parent of this object. If this parent is d this instance is
d too.

\see ~NodeInstanceView setRenderOffScreen
*/
NodeInstanceView::NodeInstanceView(QObject *parent)
        : AbstractView(parent),
    m_graphicsView(new QGraphicsView),
    m_engine(new QDeclarativeEngine(this)),
    m_blockStatePropertyChanges(false)
{
    m_graphicsView->setAttribute(Qt::WA_DontShowOnScreen, true);
    m_graphicsView->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    m_graphicsView->setScene(new QGraphicsScene(m_graphicsView.data()));

    Q_ASSERT(!m_engine.isNull());

    QDeclarativeEnginePrivate *privateQDeclarativeEngine = QDeclarativeEnginePrivate::get(m_engine.data());
    Q_ASSERT(privateQDeclarativeEngine);
    privateQDeclarativeEngine->scriptEngine.setProcessEventsInterval(100);
}


/*! \brief Destructor

*/
NodeInstanceView::~NodeInstanceView()
{
    removeAllInstanceNodeRelationships();
}

/*!   \name Overloaded Notifiers
 *  This methodes notify the view that something has happen in the model
 */
//\{
/*! \brief Notifing the view that it was attached to a model.

  For every ModelNode in the model a NodeInstance will be created.
\param model Model to which the view is attached
*/
void NodeInstanceView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    engine()->setBaseUrl(model->fileUrl());
    loadModel(model);
}

void NodeInstanceView::modelAboutToBeDetached(Model * model)
{
    removeAllInstanceNodeRelationships();
    AbstractView::modelAboutToBeDetached(model);
}


/*! \brief Notifing the view that a node was created.
  A NodeInstance will be created for the new created ModelNode.
\param createdNode New created ModelNode.
*/
void NodeInstanceView::nodeCreated(const ModelNode &createdNode)
{
    NodeInstance instance(loadNode(createdNode));
    instance.show();
}

/*! \brief Notifing the view that a node was created.
\param removedNode
*/
void NodeInstanceView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    removeInstanceAndSubInstances(removedNode);
}

void NodeInstanceView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{

}

/*! \brief Notifing the view that a AbstractProperty was added to a ModelNode.

  The property will be set for the NodeInstance.

\param state ModelNode to which the Property belongs
\param property AbstractProperty which was added
\see AbstractProperty NodeInstance ModelNode
*/

void NodeInstanceView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach (const AbstractProperty &property, propertyList) {
        resetInstanceProperty(property);

        if (property.isNodeAbstractProperty()) {
            foreach (const ModelNode &subNode, property.toNodeAbstractProperty().allSubNodes())
                removeInstanceNodeRelationship(subNode);
        }
    }
}

void NodeInstanceView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

void NodeInstanceView::resetInstanceProperty(const AbstractProperty &property)
{
    if (hasInstanceForNode(property.parentModelNode())) { // TODO ugly workaround
        NodeInstance instance = instanceForNode(property.parentModelNode());
        Q_ASSERT(instance.isValid());
        const QString name = property.name();
        if (activeStateInstance().isValid()) {
            bool statePropertyWasReseted = activeStateInstance().resetStateProperty(instance, name, instance.resetVariant(name));
            if (!statePropertyWasReseted)
                instance.resetProperty(name);
        } else {
            instance.resetProperty(name);
        }
    }
}

void NodeInstanceView::setInstancePropertyBinding(const BindingProperty &property)
{
    NodeInstance instance = instanceForNode(property.parentModelNode());

    const QString name = property.name();
    const QString expression = property.expression();


    if (activeStateInstance().isValid()) {
        bool stateBindingWasUpdated = activeStateInstance().updateStateBinding(instance, name, expression);
        if (!stateBindingWasUpdated) {
            if (property.isDynamic())
                instance.setPropertyDynamicBinding(name, property.dynamicTypeName(), expression);
            else
                instance.setPropertyBinding(name, expression);
        }
    } else {
        if (property.isDynamic())
            instance.setPropertyDynamicBinding(name, property.dynamicTypeName(), expression);
        else
            instance.setPropertyBinding(name, expression);
    }


    if (property.parentModelNode().isRootNode()
        && (name == "width" || name == "height")) {
        QGraphicsObject *rootGraphicsObject = qobject_cast<QGraphicsObject*>(instance.internalObject());
        if (rootGraphicsObject) {
            m_graphicsView->setSceneRect(rootGraphicsObject->boundingRect());
        }
    }

    instance.paintUpdate();

}

void NodeInstanceView::setInstancePropertyVariant(const VariantProperty &property)
{
    NodeInstance instance = instanceForNode(property.parentModelNode());

    const QString name = property.name();
    const QVariant value = property.value();


    if (activeStateInstance().isValid()) {
        bool stateValueWasUpdated = activeStateInstance().updateStateVariant(instance, name, value);
        if (!stateValueWasUpdated) {
            if (property.isDynamic())
                instance.setPropertyDynamicVariant(name, property.dynamicTypeName(), value);
            else
                instance.setPropertyVariant(name, value);
        }
    } else { //base state
        if (property.isDynamic())
            instance.setPropertyDynamicVariant(name, property.dynamicTypeName(), value);
        else
            instance.setPropertyVariant(name, value);
    }


    if (property.parentModelNode().isRootNode()
        && (name == "width" || name == "height")) {
        QGraphicsObject *rootGraphicsObject = qobject_cast<QGraphicsObject*>(instance.internalObject());
        if (rootGraphicsObject) {
            m_graphicsView->setSceneRect(rootGraphicsObject->boundingRect());
        }
    }

    instance.paintUpdate();
}

void NodeInstanceView::removeInstanceAndSubInstances(const ModelNode &node)
{
    foreach(const ModelNode &subNode, node.allSubModelNodes()) {
        if (hasInstanceForNode(subNode))
            removeInstanceNodeRelationship(subNode);
    }

    if (hasInstanceForNode(node))
        removeInstanceNodeRelationship(node);
}

void NodeInstanceView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    removeAllInstanceNodeRelationships();

    QList<ModelNode> nodeList;

    nodeList.append(allModelNodes());

    loadNodes(nodeList);
}

void NodeInstanceView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    foreach (const BindingProperty &property, propertyList)
        setInstancePropertyBinding(property);
}

/*! \brief Notifing the view that a AbstractProperty value was changed to a ModelNode.

  The property will be set for the NodeInstance.

\param state ModelNode to which the Property belongs
\param property AbstractProperty which was changed
\param newValue New Value of the property
\param oldValue Old Value of the property
\see AbstractProperty NodeInstance ModelNode
*/

void NodeInstanceView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    foreach (const VariantProperty &property, propertyList)
        setInstancePropertyVariant(property);
}
/*! \brief Notifing the view that a ModelNode has a new Parent.

  Note that also the ModelNode::childNodes() list was changed. The
  NodeInstance tree will be changed to reflect the ModelNode tree change.

\param node ModelNode which parent was changed.
\param oldParent Old parent of the node.
\param newParent New parent of the node.

\see NodeInstance ModelNode
*/

void NodeInstanceView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    NodeInstance nodeInstance(instanceForNode(node));
    NodeInstance oldParentInstance;
    if (hasInstanceForNode(oldPropertyParent.parentModelNode()))
        oldParentInstance = instanceForNode(oldPropertyParent.parentModelNode());
    NodeInstance newParentInstance;
    if (hasInstanceForNode(newPropertyParent.parentModelNode()))
        newParentInstance = instanceForNode(newPropertyParent.parentModelNode());
    nodeInstance.reparent(oldParentInstance, oldPropertyParent.name(), newParentInstance, newPropertyParent.name());
}

void NodeInstanceView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/)
{
    // TODO: We have to probably reload everything, so that images etc are updated!!!
    engine()->setBaseUrl(model()->fileUrl());
}

void NodeInstanceView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& /*oldId*/)
{
    if (hasInstanceForNode(node)) {
        NodeInstance instance = instanceForNode(node);

        instance.setId(newId);
    }
}

void NodeInstanceView::nodeOrderChanged(const NodeListProperty & listProperty,
                                        const ModelNode & /*movedNode*/, int /*oldIndex*/)
{
    foreach(const ModelNode &node, listProperty.toModelNodeList()) {
        NodeInstance instance = instanceForNode(node);
        if (instance.isValid())
            instance.reparent(instance.parent(), listProperty.name(), instance.parent(), listProperty.name());
    }
}

/*! \brief Notifing the view that the selection has been changed.

  Do nothing.

\param selectedNodeList List of ModelNode which has been selected
\param lastSelectedNodeList List of ModelNode which was selected

\see ModelNode NodeInstance
*/
void NodeInstanceView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/,
                                              const QList<ModelNode> &/*lastSelectedNodeList*/)
{
}


//\}

void NodeInstanceView::loadNodes(const QList<ModelNode> &nodeList)
{
    foreach (const ModelNode &node, nodeList)
        loadNode(node);

    foreach (const ModelNode &node, nodeList) {
        if (node.hasParentProperty())
            instanceForNode(node).reparent(NodeInstance(), QString(), instanceForNode(node.parentProperty().parentModelNode()), node.parentProperty().name());
    }

    foreach (const ModelNode &node, nodeList) {
        foreach (const BindingProperty &property, node.bindingProperties())
            instanceForNode(node).setPropertyBinding(property.name(), property.expression());
    }
}

// TODO: Set base state as current model state
void NodeInstanceView::loadModel(Model *model)
{
    Q_ASSERT(rootModelNode().isValid());

    removeAllInstanceNodeRelationships();

    engine()->rootContext()->setBaseUrl(model->fileUrl());

    loadNodes(allModelNodes());
}

void NodeInstanceView::removeAllInstanceNodeRelationships()
{
    // prevent destroyed() signals calling back

    //first  the root object
    if (rootNodeInstance().internalObject())
        rootNodeInstance().internalObject()->disconnect();

    rootNodeInstance().makeInvalid();


    foreach (NodeInstance instance, m_objectInstanceHash.values()) {
        if (instance.internalObject())
            instance.internalObject()->disconnect();
        instance.makeInvalid();
    }

    m_nodeInstanceHash.clear();
    m_objectInstanceHash.clear();
}

/*! \brief Returns a List of all NodeInstances

\see NodeInstance
*/

QList<NodeInstance> NodeInstanceView::instances() const
{
    return m_nodeInstanceHash.values();
}

/*! \brief Returns the NodeInstance for this ModelNode

  Returns a invalid NodeInstance if no NodeInstance for this ModelNode exists.

\param node ModelNode must be valid.
\returns  NodeStance for ModelNode.
\see NodeInstance
*/
NodeInstance NodeInstanceView::instanceForNode(const ModelNode &node)
{
    Q_ASSERT(node.isValid());
    Q_ASSERT(m_nodeInstanceHash.contains(node));
    Q_ASSERT(m_nodeInstanceHash.value(node).modelNode() == node);
    return m_nodeInstanceHash.value(node);
}

bool NodeInstanceView::hasInstanceForNode(const ModelNode &node)
{
    return m_nodeInstanceHash.contains(node);
}

NodeInstance NodeInstanceView::instanceForObject(QObject *object)
{
    if (object == 0)
        return NodeInstance();

    return m_objectInstanceHash.value(object);
}

bool NodeInstanceView::hasInstanceForObject(QObject *object)
{
    if (object == 0)
        return false;

    return m_objectInstanceHash.contains(object);
}


/*! \brief Returns the root NodeInstance of this view.


\returns  Root NodeIntance for this view.
\see NodeInstance
*/
NodeInstance NodeInstanceView::rootNodeInstance() const
{
    return m_rootNodeInstance;
}

/*! \brief Returns the view NodeInstance of this view.

  This can be the root NodeInstance if it is specified in the qml file.
\code
    QGraphicsView {
         QGraphicsScene {
             Item {}
         }
    }
\endcode

    If there is node view in the qml file:
 \code

    Item {}

\endcode
    Than there will be a new NodeInstance for this QGraphicsView
    generated which is not the root instance of this NodeInstanceView.

    This is the way to get this QGraphicsView NodeInstance.

\returns  Root NodeIntance for this view.
\see NodeInstance
*/



void NodeInstanceView::insertInstanceNodeRelationship(const ModelNode &node, const NodeInstance &instance)
{
    instance.internalObject()->installEventFilter(childrenChangeEventFilter());


    Q_ASSERT(!m_nodeInstanceHash.contains(node));
    m_nodeInstanceHash.insert(node, instance);
    m_objectInstanceHash.insert(instance.internalObject(), instance);
}

QDeclarativeEngine *NodeInstanceView::engine() const
{
    return m_engine.data();
}

Internal::ChildrenChangeEventFilter *NodeInstanceView::childrenChangeEventFilter()
{
    if (m_childrenChangeEventFilter.isNull()) {
        m_childrenChangeEventFilter = new Internal::ChildrenChangeEventFilter(this);
        connect(m_childrenChangeEventFilter.data(), SIGNAL(childrenChanged(QObject*)), this, SLOT(emitParentChanged(QObject*)));
    }

    return m_childrenChangeEventFilter.data();
}

void NodeInstanceView::removeInstanceNodeRelationship(const ModelNode &node)
{
    Q_ASSERT(m_nodeInstanceHash.contains(node));
    NodeInstance instance = instanceForNode(node);
    m_objectInstanceHash.remove(instanceForNode(node).internalObject());
    m_nodeInstanceHash.remove(node);
    instance.makeInvalid();
}

void NodeInstanceView::notifyPropertyChange(const ModelNode &node, const QString &propertyName)
{
    if (m_blockStatePropertyChanges)
        return;

    if (qmlModelView()) {
        qmlModelView()->nodeInstancePropertyChanged(ModelNode(node,qmlModelView()), propertyName);
    }
}


void NodeInstanceView::setQmlModelView(QmlModelView *qmlModelView)
{
    m_qmlModelView = qmlModelView;
}

QmlModelView *NodeInstanceView::qmlModelView() const
{
    return m_qmlModelView.data();
}

void NodeInstanceView::setBlockStatePropertyChanges(bool block)
{
    m_blockStatePropertyChanges = block;
}

void NodeInstanceView::setStateInstance(const NodeInstance &stateInstance)
{
    m_activeStateInstance = stateInstance;
}

void NodeInstanceView::clearStateInstance()
{
    m_activeStateInstance = NodeInstance();
}

NodeInstance NodeInstanceView::activeStateInstance() const
{
    return m_activeStateInstance;
}

void NodeInstanceView::emitParentChanged(QObject *child)
{
    if (hasInstanceForObject(child)) {
        notifyPropertyChange(instanceForObject(child).modelNode(), "parent");
    }
}

NodeInstance NodeInstanceView::loadNode(const ModelNode &node, QObject *objectToBeWrapped)
{
    Q_ASSERT(node.isValid());
    NodeInstance instance(NodeInstance::create(this, node, objectToBeWrapped));

    insertInstanceNodeRelationship(node, instance);

    if (node.isRootNode()) {
        m_rootNodeInstance = instance;
        QGraphicsObject *rootGraphicsObject = qobject_cast<QGraphicsObject*>(instance.internalObject());
        if (rootGraphicsObject) {
            m_graphicsView->scene()->addItem(rootGraphicsObject);
            m_graphicsView->setSceneRect(rootGraphicsObject->boundingRect());
        }
    }

    return instance;
}

void NodeInstanceView::activateState(const NodeInstance &instance)
{
    NodeInstance stateInstance(instance);
    stateInstance.activateState();
}

void NodeInstanceView::activateBaseState()
{
    if (activeStateInstance().isValid())
        activeStateInstance().deactivateState();
}

void NodeInstanceView::removeRecursiveChildRelationship(const ModelNode &removedNode)
{
    foreach (const ModelNode &childNode, removedNode.allDirectSubModelNodes())
        removeRecursiveChildRelationship(childNode);

    removeInstanceNodeRelationship(removedNode);
}

void NodeInstanceView::render(QPainter * painter, const QRectF &target, const QRectF &source, Qt::AspectRatioMode aspectRatioMode)
{
    if (m_graphicsView) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
        painter->setRenderHint(QPainter::NonCosmeticDefaultPen, true);
        m_graphicsView->scene()->render(painter, target, source, aspectRatioMode);
        painter->restore();
    }
}

QRectF NodeInstanceView::sceneRect() const
{
    if (m_graphicsView)
       return rootNodeInstance().boundingRect();

    return QRectF();
}

}
