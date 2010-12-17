/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <QFileSystemWatcher>

#include <model.h>
#include <modelnode.h>
#include <metainfo.h>

#include <typeinfo>
#include <iwidgetplugin.h>

#include "abstractproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"

#include <nodeinstanceserverinterface.h>

#include "createscenecommand.h"
#include "createinstancescommand.h"
#include "clearscenecommand.h"
#include "changefileurlcommand.h"
#include "reparentinstancescommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changeidscommand.h"
#include "removeinstancescommand.h"
#include "removepropertiescommand.h"
#include "valueschangedcommand.h"
#include "pixmapchangedcommand.h"
#include "informationchangedcommand.h"
#include "changestatecommand.h"
#include "addimportcommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"

#include "nodeinstanceserverproxy.h"

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
        : AbstractView(parent)
{
    m_resetTimer.setInterval(200);
    m_resetTimer.setSingleShot(true);
    connect(&m_resetTimer, SIGNAL(timeout()), this, SLOT(restartProcessDelayed()));
}


/*! \brief Destructor

*/
NodeInstanceView::~NodeInstanceView()
{
    removeAllInstanceNodeRelationships();
    delete nodeInstanceServer();
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
    m_nodeInstanceServer = new NodeInstanceServerProxy(this);
    connect(m_nodeInstanceServer.data(), SIGNAL(processCrashed()), this, SLOT(restartProcess()));

    setBlockUpdates(true);
    nodeInstanceServer()->createScene(createCreateSceneCommand());

    nodeInstanceServer()->changeFileUrl(createChangeFileUrlCommand(model->fileUrl()));
    foreach(const Import &import, model->imports())
        nodeInstanceServer()->addImport(createImportCommand(import));

    loadNodes(allModelNodes());

    setBlockUpdates(false);
}

void NodeInstanceView::modelAboutToBeDetached(Model * model)
{
    removeAllInstanceNodeRelationships();
    nodeInstanceServer()->clearScene(createClearSceneCommand());
    delete nodeInstanceServer();
    AbstractView::modelAboutToBeDetached(model);
}


void NodeInstanceView::restartProcess()
{
    setBlockUpdates(true);
    Model *oldModel = model();
    if (oldModel) {
        oldModel->detachView(this);
        oldModel->attachView(this);
    }
    setBlockUpdates(false);
}

void NodeInstanceView::restartProcessDelayed()
{
    m_resetTimer.start();
}

/*! \brief Notifing the view that a node was created.
  A NodeInstance will be created for the new created ModelNode.
\param createdNode New created ModelNode.
*/
void NodeInstanceView::nodeCreated(const ModelNode &createdNode)
{
    NodeInstance instance = loadNode(createdNode);
    nodeInstanceServer()->createInstances(createCreateInstancesCommand(QList<NodeInstance>() << instance));
    nodeInstanceServer()->changePropertyValues(createChangeValueCommand(createdNode.variantProperties()));
    nodeInstanceServer()->completeComponent(createComponentCompleteCommand(QList<NodeInstance>() << instance));
}

/*! \brief Notifing the view that a node was created.
\param removedNode
*/
void NodeInstanceView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    nodeInstanceServer()->removeInstances(createRemoveInstancesCommand(removedNode));
    removeInstanceAndSubInstances(removedNode);
}

void NodeInstanceView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NodeInstanceView::resetHorizontalAnchors(const ModelNode &modelNode)
{
    QList<BindingProperty> bindingList;
    QList<VariantProperty> valueList;

    if (modelNode.hasBindingProperty("x")) {
        bindingList.append(modelNode.bindingProperty("x"));
    } else if (modelNode.hasVariantProperty("x")) {
        valueList.append(modelNode.variantProperty("x"));
    }

    if (modelNode.hasBindingProperty("width")) {
        bindingList.append(modelNode.bindingProperty("width"));
    } else if (modelNode.hasVariantProperty("width")) {
        valueList.append(modelNode.variantProperty("width"));
    }

    if (!valueList.isEmpty())
        nodeInstanceServer()->changePropertyValues(createChangeValueCommand(valueList));

    if (!bindingList.isEmpty())
        nodeInstanceServer()->changePropertyBindings(createChangeBindingCommand(bindingList));

}

void NodeInstanceView::resetVerticalAnchors(const ModelNode &modelNode)
{
    QList<BindingProperty> bindingList;
    QList<VariantProperty> valueList;

    if (modelNode.hasBindingProperty("yx")) {
        bindingList.append(modelNode.bindingProperty("yx"));
    } else if (modelNode.hasVariantProperty("y")) {
        valueList.append(modelNode.variantProperty("y"));
    }

    if (modelNode.hasBindingProperty("height")) {
        bindingList.append(modelNode.bindingProperty("height"));
    } else if (modelNode.hasVariantProperty("height")) {
        valueList.append(modelNode.variantProperty("height"));
    }

    if (!valueList.isEmpty())
        nodeInstanceServer()->changePropertyValues(createChangeValueCommand(valueList));

    if (!bindingList.isEmpty())
        nodeInstanceServer()->changePropertyBindings(createChangeBindingCommand(bindingList));
}

void NodeInstanceView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{

    QList<ModelNode> nodeList;
    QList<AbstractProperty> nonNodePropertyList;

    foreach (const AbstractProperty &property, propertyList) {
        if (property.isNodeAbstractProperty()) {
            nodeList.append(property.toNodeAbstractProperty().allSubNodes());
        } else {
            nonNodePropertyList.append(property);
        }
    }

    nodeInstanceServer()->removeInstances(createRemoveInstancesCommand(nodeList));
    nodeInstanceServer()->removeProperties(createRemovePropertiesCommand(nonNodePropertyList));

    foreach (const AbstractProperty &property, propertyList) {
        const QString &name = property.name();
        if (name == "anchors.fill") {
            resetHorizontalAnchors(property.parentModelNode());
            resetVerticalAnchors(property.parentModelNode());
        } else if (name == "anchors.centerIn") {
            resetHorizontalAnchors(property.parentModelNode());
            resetVerticalAnchors(property.parentModelNode());
        } else if (name == "anchors.top") {
            resetVerticalAnchors(property.parentModelNode());
        } else if (name == "anchors.left") {
            resetHorizontalAnchors(property.parentModelNode());
        } else if (name == "anchors.right") {
            resetHorizontalAnchors(property.parentModelNode());
        } else if (name == "anchors.bottom") {
            resetVerticalAnchors(property.parentModelNode());
        } else if (name == "anchors.horizontalCenter") {
            resetHorizontalAnchors(property.parentModelNode());
        } else if (name == "anchors.verticalCenter") {
            resetVerticalAnchors(property.parentModelNode());
        } else if (name == "anchors.baseline") {
            resetVerticalAnchors(property.parentModelNode());
        }
    }

    foreach (const ModelNode &node, nodeList)
        removeInstanceNodeRelationship(node);
}

void NodeInstanceView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
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
    nodeInstanceServer()->clearScene(createClearSceneCommand());
    removeAllInstanceNodeRelationships();

    QList<ModelNode> nodeList;

    nodeList.append(allModelNodes());

    nodeInstanceServer()->createScene(createCreateSceneCommand());
    loadNodes(nodeList);
}

void NodeInstanceView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    nodeInstanceServer()->changePropertyBindings(createChangeBindingCommand(propertyList));
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
    nodeInstanceServer()->changePropertyValues(createChangeValueCommand(propertyList));
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
    nodeInstanceServer()->reparentInstances(createReparentInstancesCommand(node, newPropertyParent, oldPropertyParent));
//    NodeInstance nodeInstance(instanceForNode(node));
//    NodeInstance oldParentInstance;
//    if (hasInstanceForNode(oldPropertyParent.parentModelNode()))
//        oldParentInstance = instanceForNode(oldPropertyParent.parentModelNode());
//    NodeInstance newParentInstance;
//    if (hasInstanceForNode(newPropertyParent.parentModelNode()))
//        newParentInstance = instanceForNode(newPropertyParent.parentModelNode());
//    nodeInstance.reparent(oldParentInstance, oldPropertyParent.name(), newParentInstance, newPropertyParent.name());
}

void NodeInstanceView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}


void NodeInstanceView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &newUrl)
{
    // TODO: We have to probably reload everything, so that images etc are updated!!!
    //engine()->setBaseUrl(model()->fileUrl());

    //TODO reload the whole scene
    nodeInstanceServer()->changeFileUrl(createChangeFileUrlCommand(newUrl));
}

void NodeInstanceView::nodeIdChanged(const ModelNode& node, const QString& /*newId*/, const QString& /*oldId*/)
{
    if (hasInstanceForNode(node)) {
        NodeInstance instance = instanceForNode(node);
        nodeInstanceServer()->changeIds(createChangeIdsCommand(QList<NodeInstance>() << instance));
    }
}

void NodeInstanceView::nodeOrderChanged(const NodeListProperty & listProperty,
                                        const ModelNode & /*movedNode*/, int /*oldIndex*/)
{
    QVector<ReparentContainer> containerList;
    QString propertyName = listProperty.name();
    qint32 containerInstanceId = -1;
    ModelNode containerNode = listProperty.parentModelNode();
    if (hasInstanceForNode(containerNode))
        containerInstanceId = instanceForNode(containerNode).instanceId();

    foreach(const ModelNode &node, listProperty.toModelNodeList()) {
        qint32 instanceId = -1;
        if (hasInstanceForNode(node)) {
            instanceId = instanceForNode(node).instanceId();
            ReparentContainer container(instanceId, containerInstanceId, propertyName, containerInstanceId, propertyName);
            containerList.append(container);
        }
    }

    nodeInstanceServer()->reparentInstances(ReparentInstancesCommand(containerList));
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

void NodeInstanceView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{

}

void NodeInstanceView::instancePropertyChange(const QList<QPair<ModelNode, QString> > &/*propertyList*/)
{

}

void NodeInstanceView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

void NodeInstanceView::importAdded(const Import & /*import*/)
{
    restartProcessDelayed();
}

void NodeInstanceView::importRemoved(const Import &/*import*/)
{
    restartProcessDelayed();
}

//\}


void NodeInstanceView::loadNodes(const QList<ModelNode> &nodeList)
{
    QList<NodeInstance> instanceList;

    foreach (const ModelNode &node, nodeList)
        instanceList.append(loadNode(node));


    QList<VariantProperty> variantPropertyList;
    QList<BindingProperty> bindingPropertyList;

    foreach (const ModelNode &node, nodeList) {
        variantPropertyList.append(node.variantProperties());
        bindingPropertyList.append(node.bindingProperties());
    }

    nodeInstanceServer()->createInstances(createCreateInstancesCommand(instanceList));
    nodeInstanceServer()->reparentInstances(createReparentInstancesCommand(instanceList));
    nodeInstanceServer()->changeIds(createChangeIdsCommand(instanceList));
    nodeInstanceServer()->changePropertyValues(createChangeValueCommand(variantPropertyList));
    nodeInstanceServer()->changePropertyBindings(createChangeBindingCommand(bindingPropertyList));
    nodeInstanceServer()->completeComponent(createComponentCompleteCommand(instanceList));
}

void NodeInstanceView::removeAllInstanceNodeRelationships()
{
    // prevent destroyed() signals calling back

//    foreach (NodeInstance instance, m_objectInstanceHash.values()) {
//        if (instance.isValid())
//            instance.setId(QString());
//    }

//    //first  the root object
//    if (rootNodeInstance().internalObject())
//        rootNodeInstance().internalObject()->disconnect();

//    rootNodeInstance().makeInvalid();


    foreach (NodeInstance instance, m_idInstanceHash.values()) {
//        if (instance.internalObject())
//            instance.internalObject()->disconnect();
        instance.makeInvalid();
        }

    m_nodeInstanceHash.clear();
    m_idInstanceHash.clear();
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
NodeInstance NodeInstanceView::instanceForNode(const ModelNode &node) const
{
    Q_ASSERT(node.isValid());
    Q_ASSERT(m_nodeInstanceHash.contains(node));
    Q_ASSERT(m_nodeInstanceHash.value(node).modelNode() == node);
    return m_nodeInstanceHash.value(node);
}

bool NodeInstanceView::hasInstanceForNode(const ModelNode &node) const
{
    return m_nodeInstanceHash.contains(node);
}

NodeInstance NodeInstanceView::instanceForId(qint32 id) const
{
    if (id < 0)
        return NodeInstance();

    return m_idInstanceHash.value(id);
}

bool NodeInstanceView::hasInstanceForId(qint32 id) const
{
    if (id < 0)
        return false;

    return m_idInstanceHash.contains(id);
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



void NodeInstanceView::insertInstanceRelationships(const NodeInstance &instance)
{
    Q_ASSERT(instance.instanceId() >=0);
    Q_ASSERT(!m_nodeInstanceHash.contains(instance.modelNode()));
    Q_ASSERT(!m_idInstanceHash.contains(instance.instanceId()));
    m_nodeInstanceHash.insert(instance.modelNode(), instance);
    m_idInstanceHash.insert(instance.instanceId(), instance);
}

void NodeInstanceView::removeInstanceNodeRelationship(const ModelNode &node)
{
    Q_ASSERT(m_nodeInstanceHash.contains(node));
    NodeInstance instance = instanceForNode(node);
//    if (instance.isValid())
//        instance.setId(QString());
    m_idInstanceHash.remove(instanceForNode(node).instanceId());
    m_nodeInstanceHash.remove(node);
    instance.makeInvalid();
}

void NodeInstanceView::setBlockUpdates(bool block)
{
    if (m_blockUpdates == 0 && block == true)
        m_nodeInstanceServer->setBlockUpdates(true);

    if (block) {
        m_blockUpdates++;
    } else if (m_blockUpdates > 0) {
        m_blockUpdates--;
    }
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

NodeInstanceServerInterface *NodeInstanceView::nodeInstanceServer() const
{
    return m_nodeInstanceServer.data();
}


NodeInstance NodeInstanceView::loadNode(const ModelNode &node)
{
    NodeInstance instance(NodeInstance::create(node));

    insertInstanceRelationships(instance);

    if (node.isRootNode()) {
        m_rootNodeInstance = instance;
    }

    return instance;
}

void NodeInstanceView::activateState(const NodeInstance &instance)
{
    nodeInstanceServer()->changeState(ChangeStateCommand(instance.instanceId()));
//    activateBaseState();
//    NodeInstance stateInstance(instance);
//    stateInstance.activateState();
}

void NodeInstanceView::activateBaseState()
{
    nodeInstanceServer()->changeState(ChangeStateCommand(-1));
//    if (activeStateInstance().isValid())
//        activeStateInstance().deactivateState();
}

void NodeInstanceView::removeRecursiveChildRelationship(const ModelNode &removedNode)
{
//    if (hasInstanceForNode(removedNode)) {
//        instanceForNode(removedNode).setId(QString());
//    }

    foreach (const ModelNode &childNode, removedNode.allDirectSubModelNodes())
        removeRecursiveChildRelationship(childNode);

    removeInstanceNodeRelationship(removedNode);
}

QRectF NodeInstanceView::sceneRect() const
{
    if (rootNodeInstance().isValid())
       return rootNodeInstance().boundingRect();

    return QRectF();
}

CreateSceneCommand NodeInstanceView::createCreateSceneCommand() const
{
    return CreateSceneCommand();
}
ClearSceneCommand NodeInstanceView::createClearSceneCommand() const
{
    return ClearSceneCommand();
}

CompleteComponentCommand NodeInstanceView::createComponentCompleteCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<qint32> containerList;
    foreach(const NodeInstance &instance, instanceList) {
        if (instance.instanceId() >= 0)
            containerList.append(instance.instanceId());
    }

    return CompleteComponentCommand(containerList);
}

ComponentCompletedCommand NodeInstanceView::createComponentCompletedCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<qint32> containerList;
    foreach(const NodeInstance &instance, instanceList) {
        if (instance.instanceId() >= 0)
            containerList.append(instance.instanceId());
    }

    return ComponentCompletedCommand(containerList);
}

CreateInstancesCommand NodeInstanceView::createCreateInstancesCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<InstanceContainer> containerList;
    foreach(const NodeInstance &instance, instanceList) {
        InstanceContainer container(instance.instanceId(), instance.modelNode().type(), instance.modelNode().majorVersion(), instance.modelNode().minorVersion(), instance.modelNode().metaInfo().componentFileName());
        containerList.append(container);
    }

    return CreateInstancesCommand(containerList);
}

ReparentInstancesCommand NodeInstanceView::createReparentInstancesCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<ReparentContainer> containerList;
    foreach(const NodeInstance &instance, instanceList) {
        if (instance.modelNode().hasParentProperty()) {
            NodeAbstractProperty parentProperty = instance.modelNode().parentProperty();
            ReparentContainer container(instance.instanceId(), -1, QString(), instanceForNode(parentProperty.parentModelNode()).instanceId(), parentProperty.name());
            containerList.append(container);
        }
    }

    return ReparentInstancesCommand(containerList);
}

ReparentInstancesCommand NodeInstanceView::createReparentInstancesCommand(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent) const
{
    QVector<ReparentContainer> containerList;

    qint32 newParentInstanceId = -1;
    qint32 oldParentInstanceId = -1;

    if (newPropertyParent.isValid() && hasInstanceForNode(newPropertyParent.parentModelNode()))
        newParentInstanceId = instanceForNode(newPropertyParent.parentModelNode()).instanceId();


    if (oldPropertyParent.isValid() && hasInstanceForNode(oldPropertyParent.parentModelNode()))
        oldParentInstanceId = instanceForNode(oldPropertyParent.parentModelNode()).instanceId();


    ReparentContainer container(instanceForNode(node).instanceId(), oldParentInstanceId, oldPropertyParent.name(), newParentInstanceId, newPropertyParent.name());

    containerList.append(container);

    return ReparentInstancesCommand(containerList);
}

ChangeFileUrlCommand NodeInstanceView::createChangeFileUrlCommand(const QUrl &fileUrl) const
{
    return ChangeFileUrlCommand(fileUrl);
}

ChangeValuesCommand NodeInstanceView::createChangeValueCommand(const QList<VariantProperty>& propertyList) const
{
    QVector<PropertyValueContainer> containerList;

    foreach(const VariantProperty &property, propertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            PropertyValueContainer container(instance.instanceId(), property.name(), property.value(), property.dynamicTypeName());
            containerList.append(container);
        }

    }

    return ChangeValuesCommand(containerList);
}

ChangeBindingsCommand NodeInstanceView::createChangeBindingCommand(const QList<BindingProperty> &propertyList) const
{
    QVector<PropertyBindingContainer> containerList;

    foreach(const BindingProperty &property, propertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            PropertyBindingContainer container(instance.instanceId(), property.name(), property.expression(), property.dynamicTypeName());
            containerList.append(container);
        }

    }

    return ChangeBindingsCommand(containerList);
}

ChangeIdsCommand NodeInstanceView::createChangeIdsCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<IdContainer> containerList;
    foreach(const NodeInstance &instance, instanceList) {
        QString id = instance.modelNode().id();
        if (!id.isEmpty()) {
            IdContainer container(instance.instanceId(), id);
            containerList.append(container);
        }
    }

    return ChangeIdsCommand(containerList);
}



RemoveInstancesCommand NodeInstanceView::createRemoveInstancesCommand(const QList<ModelNode> &nodeList) const
{
    QVector<qint32> idList;
    foreach(const ModelNode &node, nodeList) {
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);

            if (instance.instanceId() >= 0) {
                idList.append(instance.instanceId());
            }
        }
    }

    return RemoveInstancesCommand(idList);
}

RemoveInstancesCommand NodeInstanceView::createRemoveInstancesCommand(const ModelNode &node) const
{
    QVector<qint32> idList;

    if (node.isValid() && hasInstanceForNode(node))
        idList.append(instanceForNode(node).instanceId());

    return RemoveInstancesCommand(idList);
}

RemovePropertiesCommand NodeInstanceView::createRemovePropertiesCommand(const QList<AbstractProperty> &propertyList) const
{
    QVector<PropertyAbstractContainer> containerList;

    foreach(const AbstractProperty &property, propertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            PropertyAbstractContainer container(instance.instanceId(), property.name(), property.dynamicTypeName());
            containerList.append(container);
        }

    }

    return RemovePropertiesCommand(containerList);
}

AddImportCommand NodeInstanceView::createImportCommand(const Import &import)
{
    return AddImportCommand(import.url(), import.file(), import.version(), import.alias(), import.importPaths());
}

void NodeInstanceView::valuesChanged(const ValuesChangedCommand &command)
{
    if (!model())
        return;

    QList<QPair<ModelNode, QString> > valuePropertyChangeList;

    foreach(const PropertyValueContainer &container, command.valueChanges()) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instance.setProperty(container.name(), container.value());
                valuePropertyChangeList.append(qMakePair(instance.modelNode(), container.name()));
            }
        }
    }

    if (!valuePropertyChangeList.isEmpty())
        emitInstancePropertyChange(valuePropertyChangeList);
}

void NodeInstanceView::pixmapChanged(const PixmapChangedCommand &command)
{
      if (!model())
        return;

    QSet<ModelNode> renderImageChangeSet;

    foreach (const ImageContainer &container, command.images()) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instance.setRenderImage(container.image());
                renderImageChangeSet.insert(instance.modelNode());
            }
        }
    }

    if (!renderImageChangeSet.isEmpty())
         emitCustomNotification("__instance render pixmap changed__", renderImageChangeSet.toList());
}

void NodeInstanceView::informationChanged(const InformationChangedCommand &command)
{
    if (!model())
        return;

    QList<ModelNode> informationChangedList;

    foreach(const InformationContainer &container, command.informations()) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instance.setInformation(container.name(), container.information(), container.secondInformation(), container.thirdInformation());
                informationChangedList.append(instance.modelNode());
            }
        }
    }

    if (!informationChangedList.isEmpty())
        emitCustomNotification("__instance information changed__", informationChangedList);
}

void NodeInstanceView::statePreviewImagesChanged(const StatePreviewImageChangedCommand &/*command*/)
{

}

void NodeInstanceView::componentCompleted(const ComponentCompletedCommand &command)
{
    if (!model())
        return;

    QVector<ModelNode> nodeVector;

    foreach(const qint32 &instanceId, command.instances()) {
        if (hasModelNodeForInternalId(instanceId)) {
            nodeVector.append(modelNodeForInternalId(instanceId));
        }
    }

    if (!nodeVector.isEmpty())
        emitInstancesCompleted(nodeVector);
}

void NodeInstanceView::childrenChanged(const ChildrenChangedCommand &command)
{
     if (!model())
        return;

    QList<ModelNode> childNodeList;

    foreach(qint32 instanceId, command.childrenInstances()) {
        if (hasInstanceForId(instanceId)) {
            NodeInstance instance = instanceForId(instanceId);
            instance.setParentId(command.parentInstanceId());
            childNodeList.append(instance.modelNode());
        }
    }

    if (!childNodeList.isEmpty())
        emitCustomNotification("__instance children changed__", childNodeList);
}

}
