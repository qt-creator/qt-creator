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

#include "nodeinstanceview.h"

#include <QUrl>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QFileSystemWatcher>
#include <QMultiHash>

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
#include "changeauxiliarycommand.h"
#include "changebindingscommand.h"
#include "changeidscommand.h"
#include "changenodesourcecommand.h"
#include "removeinstancescommand.h"
#include "removepropertiescommand.h"
#include "valueschangedcommand.h"
#include "pixmapchangedcommand.h"
#include "informationchangedcommand.h"
#include "changestatecommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "tokencommand.h"
#include "removesharedmemorycommand.h"

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
NodeInstanceView::NodeInstanceView(QObject *parent, NodeInstanceServerInterface::RunModus runModus)
        : AbstractView(parent),
          m_baseStatePreviewImage(QSize(100, 100), QImage::Format_ARGB32),
          m_runModus(runModus)
{
    m_baseStatePreviewImage.fill(0xFFFFFF);
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

bool isSkippedRootNode(const ModelNode &node)
{
    static QStringList skipList =  QStringList() << "Qt.ListModel" << "QtQuick.ListModel" << "Qt.ListModel" << "QtQuick.ListModel";

    if (skipList.contains(node.type()))
        return true;

    return false;
}


bool isSkippedNode(const ModelNode &node)
{
    static QStringList skipList =  QStringList() << "QtQuick.XmlRole" << "Qt.XmlRole" << "QtQuick.ListElement" << "Qt.ListElement";

    if (skipList.contains(node.type()))
        return true;

    return false;
}

void NodeInstanceView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    m_nodeInstanceServer = new NodeInstanceServerProxy(this, m_runModus, m_pathToQt);
    m_lastCrashTime.start();
    connect(m_nodeInstanceServer.data(), SIGNAL(processCrashed()), this, SLOT(handleChrash()));

    if (!isSkippedRootNode(rootModelNode()))
        nodeInstanceServer()->createScene(createCreateSceneCommand());

    ModelNode stateNode = actualStateNode();
    if (stateNode.isValid() && stateNode.metaInfo().isSubclassOf("QtQuick.State", 1, 0)) {
        NodeInstance newStateInstance = instanceForNode(stateNode);
        activateState(newStateInstance);
    }

}

void NodeInstanceView::modelAboutToBeDetached(Model * model)
{
    removeAllInstanceNodeRelationships();
    nodeInstanceServer()->clearScene(createClearSceneCommand());
    delete nodeInstanceServer();
    m_statePreviewImage.clear();
    m_baseStatePreviewImage = QImage();
    removeAllInstanceNodeRelationships();
    m_activeStateInstance = NodeInstance();
    m_rootNodeInstance = NodeInstance();
    AbstractView::modelAboutToBeDetached(model);
}

void NodeInstanceView::handleChrash()
{
    int elaspsedTimeSinceLastCrash = m_lastCrashTime.restart();

    if (elaspsedTimeSinceLastCrash > 2000)
        restartProcess();
    else
        emit  qmlPuppetCrashed();
}



void NodeInstanceView::restartProcess()
{
    if (model()) {
        delete nodeInstanceServer();

        m_nodeInstanceServer = new NodeInstanceServerProxy(this, m_runModus, m_pathToQt);
        connect(m_nodeInstanceServer.data(), SIGNAL(processCrashed()), this, SLOT(handleChrash()));

        if (!isSkippedRootNode(rootModelNode()))
            nodeInstanceServer()->createScene(createCreateSceneCommand());

        ModelNode stateNode = actualStateNode();
        if (stateNode.isValid() && stateNode.metaInfo().isSubclassOf("QtQuick.State", 1, 0)) {
            NodeInstance newStateInstance = instanceForNode(stateNode);
            activateState(newStateInstance);
        }
    }
}

void NodeInstanceView::nodeCreated(const ModelNode &createdNode)
{
    NodeInstance instance = loadNode(createdNode);

    if (isSkippedNode(createdNode))
        return;

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
    nodeInstanceServer()->removeSharedMemory(createRemoveSharedMemoryCommand("Image", removedNode.internalId()));
    removeInstanceAndSubInstances(removedNode);
}

void NodeInstanceView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NodeInstanceView::resetHorizontalAnchors(const ModelNode &modelNode)
{
    QList<BindingProperty> bindingList;
    QList<VariantProperty> valueList;

    if (modelNode.hasBindingProperty("x"))
        bindingList.append(modelNode.bindingProperty("x"));
    else if (modelNode.hasVariantProperty("x"))
        valueList.append(modelNode.variantProperty("x"));

    if (modelNode.hasBindingProperty("width"))
        bindingList.append(modelNode.bindingProperty("width"));
    else if (modelNode.hasVariantProperty("width"))
        valueList.append(modelNode.variantProperty("width"));

    if (!valueList.isEmpty())
        nodeInstanceServer()->changePropertyValues(createChangeValueCommand(valueList));

    if (!bindingList.isEmpty())
        nodeInstanceServer()->changePropertyBindings(createChangeBindingCommand(bindingList));

}

void NodeInstanceView::resetVerticalAnchors(const ModelNode &modelNode)
{
    QList<BindingProperty> bindingList;
    QList<VariantProperty> valueList;

    if (modelNode.hasBindingProperty("yx"))
        bindingList.append(modelNode.bindingProperty("yx"));
    else if (modelNode.hasVariantProperty("y"))
        valueList.append(modelNode.variantProperty("y"));

    if (modelNode.hasBindingProperty("height"))
        bindingList.append(modelNode.bindingProperty("height"));
    else if (modelNode.hasVariantProperty("height"))
        valueList.append(modelNode.variantProperty("height"));

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
        if (property.isNodeAbstractProperty())
            nodeList.append(property.toNodeAbstractProperty().allSubNodes());
        else
            nonNodePropertyList.append(property);
    }

    nodeInstanceServer()->removeInstances(createRemoveInstancesCommand(nodeList));
    nodeInstanceServer()->removeSharedMemory(createRemoveSharedMemoryCommand("Image", nodeList));
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
    foreach (const ModelNode &subNode, node.allSubModelNodes()) {
        if (hasInstanceForNode(subNode))
            removeInstanceNodeRelationship(subNode);
    }

    if (hasInstanceForNode(node))
        removeInstanceNodeRelationship(node);
}

void NodeInstanceView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    restartProcess();
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
    if (!isSkippedNode(node))
        nodeInstanceServer()->reparentInstances(createReparentInstancesCommand(node, newPropertyParent, oldPropertyParent));
}

void NodeInstanceView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}


void NodeInstanceView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &newUrl)
{
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

    foreach (const ModelNode &node, listProperty.toModelNodeList()) {
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

void NodeInstanceView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    restartProcess();
}

void NodeInstanceView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{

}

void NodeInstanceView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void NodeInstanceView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void NodeInstanceView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void NodeInstanceView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{

}

void NodeInstanceView::auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data)
{
    if ((node.isRootNode() && (name == "width" || name == "height")) || name.endsWith(QLatin1String("@NodeInstance"))) {
        if (hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            QVariant value = data;
            if (value.isValid()) {
                PropertyValueContainer container(instance.instanceId(), name, value, QString());
                ChangeAuxiliaryCommand changeAuxiliaryCommand(QVector<PropertyValueContainer>() << container);
                nodeInstanceServer()->changeAuxiliaryValues(changeAuxiliaryCommand);
            } else {
                if (node.hasVariantProperty(name)) {
                    PropertyValueContainer container(instance.instanceId(), name, node.variantProperty(name).value(), QString());
                    ChangeValuesCommand changeValueCommand(QVector<PropertyValueContainer>() << container);
                    nodeInstanceServer()->changePropertyValues(changeValueCommand);
                } else if (node.hasBindingProperty(name)) {
                    PropertyBindingContainer container(instance.instanceId(), name, node.bindingProperty(name).expression(), QString());
                    ChangeBindingsCommand changeValueCommand(QVector<PropertyBindingContainer>() << container);
                    nodeInstanceServer()->changePropertyBindings(changeValueCommand);
                }
            }
        }
    }
}

void NodeInstanceView::customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &, const QList<QVariant> &)
{
    if (view && identifier == QLatin1String("reset QmlPuppet"))
        restartProcess();
}

void NodeInstanceView::nodeSourceChanged(const ModelNode &node, const QString & newNodeSource)
{
     if (hasInstanceForNode(node)) {
         NodeInstance instance = instanceForNode(node);
         ChangeNodeSourceCommand changeNodeSourceCommand(instance.instanceId(), newNodeSource);
         nodeInstanceServer()->changeNodeSource(changeNodeSourceCommand);
     }
}

void NodeInstanceView::rewriterBeginTransaction()
{

}

void NodeInstanceView::rewriterEndTransaction()
{

}

void NodeInstanceView::actualStateChanged(const ModelNode &node)
{
    NodeInstance newStateInstance = instanceForNode(node);

    if (newStateInstance.isValid() && node.metaInfo().isSubclassOf("QtQuick.State", 1, 0))
        nodeInstanceView()->activateState(newStateInstance);
    else
        nodeInstanceView()->activateBaseState();
}


//\}


void NodeInstanceView::removeAllInstanceNodeRelationships()
{
    m_nodeInstanceHash.clear();
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

NodeInstance NodeInstanceView::instanceForId(qint32 id)
{
    if (id < 0 || !hasModelNodeForInternalId(id))
        return NodeInstance();

    return m_nodeInstanceHash.value(modelNodeForInternalId(id));
}

bool NodeInstanceView::hasInstanceForId(qint32 id)
{
    if (id < 0 || !hasModelNodeForInternalId(id))
        return false;

    return m_nodeInstanceHash.contains(modelNodeForInternalId(id));
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
    if (m_nodeInstanceHash.contains(instance.modelNode()))
        return;

    m_nodeInstanceHash.insert(instance.modelNode(), instance);
}

void NodeInstanceView::removeInstanceNodeRelationship(const ModelNode &node)
{
    Q_ASSERT(m_nodeInstanceHash.contains(node));
    NodeInstance instance = instanceForNode(node);
    m_nodeInstanceHash.remove(node);
    instance.makeInvalid();
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

    if (node.isRootNode())
        m_rootNodeInstance = instance;

    return instance;
}

void NodeInstanceView::activateState(const NodeInstance &instance)
{
    nodeInstanceServer()->changeState(ChangeStateCommand(instance.instanceId()));
}

void NodeInstanceView::activateBaseState()
{
    nodeInstanceServer()->changeState(ChangeStateCommand(-1));
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

QList<ModelNode> filterNodesForSkipItems(const QList<ModelNode> &nodeList)
{
    QList<ModelNode> filteredNodeList;
    foreach (const ModelNode &node, nodeList) {
        if (isSkippedNode(node))
            continue;

        filteredNodeList.append(node);
    }

    return filteredNodeList;
}

CreateSceneCommand NodeInstanceView::createCreateSceneCommand()
{
    QList<ModelNode> nodeList = allModelNodes();
    QList<NodeInstance> instanceList;

    foreach (const ModelNode &node, nodeList) {
        NodeInstance instance = loadNode(node);
        if (!isSkippedNode(node))
            instanceList.append(instance);
    }

    nodeList = filterNodesForSkipItems(nodeList);

    QList<VariantProperty> variantPropertyList;
    QList<BindingProperty> bindingPropertyList;

    QVector<PropertyValueContainer> auxiliaryContainerVector;
    foreach (const ModelNode &node, nodeList) {
        variantPropertyList.append(node.variantProperties());
        bindingPropertyList.append(node.bindingProperties());
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            QHashIterator<QString, QVariant> auxiliaryIterator(node.auxiliaryData());
            while (auxiliaryIterator.hasNext()) {
                auxiliaryIterator.next();
                PropertyValueContainer container(instance.instanceId(), auxiliaryIterator.key(), auxiliaryIterator.value(), QString());
                auxiliaryContainerVector.append(container);
            }
        }
    }


    QVector<InstanceContainer> instanceContainerList;
    foreach (const NodeInstance &instance, instanceList) {
        InstanceContainer::NodeSourceType nodeSourceType = static_cast<InstanceContainer::NodeSourceType>(instance.modelNode().nodeSourceType());

        InstanceContainer::NodeMetaType nodeMetaType = InstanceContainer::ObjectMetaType;
        if (instance.modelNode().metaInfo().isSubclassOf("QtQuick.Item", -1, -1))
            nodeMetaType = InstanceContainer::ItemMetaType;

        InstanceContainer container(instance.instanceId(),
                                    instance.modelNode().type(),
                                    instance.modelNode().majorVersion(),
                                    instance.modelNode().minorVersion(),
                                    instance.modelNode().metaInfo().componentFileName(),
                                    instance.modelNode().nodeSource(),
                                    nodeSourceType,
                                    nodeMetaType
                                   );

        instanceContainerList.append(container);
    }

    QVector<ReparentContainer> reparentContainerList;
    foreach (const NodeInstance &instance, instanceList) {
        if (instance.modelNode().hasParentProperty()) {
            NodeAbstractProperty parentProperty = instance.modelNode().parentProperty();
            ReparentContainer container(instance.instanceId(), -1, QString(), instanceForNode(parentProperty.parentModelNode()).instanceId(), parentProperty.name());
            reparentContainerList.append(container);
        }
    }

    QVector<IdContainer> idContainerList;
    foreach (const NodeInstance &instance, instanceList) {
        QString id = instance.modelNode().id();
        if (!id.isEmpty()) {
            IdContainer container(instance.instanceId(), id);
            idContainerList.append(container);
        }
    }

    QVector<PropertyValueContainer> valueContainerList;
    foreach (const VariantProperty &property, variantPropertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            PropertyValueContainer container(instance.instanceId(), property.name(), property.value(), property.dynamicTypeName());
            valueContainerList.append(container);
        }
    }

    QVector<PropertyBindingContainer> bindingContainerList;
    foreach (const BindingProperty &property, bindingPropertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            PropertyBindingContainer container(instance.instanceId(), property.name(), property.expression(), property.dynamicTypeName());
            bindingContainerList.append(container);
        }
    }

    QVector<AddImportContainer> importVector;
    foreach (const Import &import, model()->imports())
        importVector.append(AddImportContainer(import.url(), import.file(), import.version(), import.alias(), import.importPaths()));

    return CreateSceneCommand(instanceContainerList,
                              reparentContainerList,
                              idContainerList,
                              valueContainerList,
                              bindingContainerList,
                              auxiliaryContainerVector,
                              importVector,
                              model()->fileUrl());
}

ClearSceneCommand NodeInstanceView::createClearSceneCommand() const
{
    return ClearSceneCommand();
}

CompleteComponentCommand NodeInstanceView::createComponentCompleteCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<qint32> containerList;
    foreach (const NodeInstance &instance, instanceList) {
        if (instance.instanceId() >= 0)
            containerList.append(instance.instanceId());
    }

    return CompleteComponentCommand(containerList);
}

ComponentCompletedCommand NodeInstanceView::createComponentCompletedCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<qint32> containerList;
    foreach (const NodeInstance &instance, instanceList) {
        if (instance.instanceId() >= 0)
            containerList.append(instance.instanceId());
    }

    return ComponentCompletedCommand(containerList);
}

CreateInstancesCommand NodeInstanceView::createCreateInstancesCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<InstanceContainer> containerList;
    foreach (const NodeInstance &instance, instanceList) {
        InstanceContainer::NodeSourceType nodeSourceType = static_cast<InstanceContainer::NodeSourceType>(instance.modelNode().nodeSourceType());

        InstanceContainer::NodeMetaType nodeMetaType = InstanceContainer::ObjectMetaType;
        if (instance.modelNode().metaInfo().isSubclassOf("QtQuick.Item", -1, -1))
            nodeMetaType = InstanceContainer::ItemMetaType;

        InstanceContainer container(instance.instanceId(), instance.modelNode().type(), instance.modelNode().majorVersion(), instance.modelNode().minorVersion(),
                                    instance.modelNode().metaInfo().componentFileName(), instance.modelNode().nodeSource(), nodeSourceType, nodeMetaType);
        containerList.append(container);
    }

    return CreateInstancesCommand(containerList);
}

ReparentInstancesCommand NodeInstanceView::createReparentInstancesCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<ReparentContainer> containerList;
    foreach (const NodeInstance &instance, instanceList) {
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

    foreach (const VariantProperty &property, propertyList) {
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

    foreach (const BindingProperty &property, propertyList) {
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
    foreach (const NodeInstance &instance, instanceList) {
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
    foreach (const ModelNode &node, nodeList) {
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);

            if (instance.instanceId() >= 0)
                idList.append(instance.instanceId());
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

    foreach (const AbstractProperty &property, propertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForNode(node)) {
            NodeInstance instance = instanceForNode(node);
            PropertyAbstractContainer container(instance.instanceId(), property.name(), property.dynamicTypeName());
            containerList.append(container);
        }

    }

    return RemovePropertiesCommand(containerList);
}

RemoveSharedMemoryCommand NodeInstanceView::createRemoveSharedMemoryCommand(const QString &sharedMemoryTypeName, quint32 keyNumber)
{
    return RemoveSharedMemoryCommand(sharedMemoryTypeName, QVector<qint32>() << keyNumber);
}

RemoveSharedMemoryCommand NodeInstanceView::createRemoveSharedMemoryCommand(const QString &sharedMemoryTypeName, const QList<ModelNode> &nodeList)
{
    QVector<qint32> keyNumberVector;

    foreach (const ModelNode &modelNode, nodeList)
        keyNumberVector.append(modelNode.internalId());

    return RemoveSharedMemoryCommand(sharedMemoryTypeName, keyNumberVector);
}

void NodeInstanceView::valuesChanged(const ValuesChangedCommand &command)
{
    if (!model())
        return;

    QList<QPair<ModelNode, QString> > valuePropertyChangeList;

    foreach (const PropertyValueContainer &container, command.valueChanges()) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instance.setProperty(container.name(), container.value());
                valuePropertyChangeList.append(qMakePair(instance.modelNode(), container.name()));
            }
        }
    }

    nodeInstanceServer()->removeSharedMemory(createRemoveSharedMemoryCommand(QLatin1String("Values"), command.keyNumber()));

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
                instance.setRenderPixmap(container.image());
                renderImageChangeSet.insert(instance.modelNode());
            }
        }
    }

    if (!renderImageChangeSet.isEmpty())
        emitInstancesRenderImageChanged(renderImageChangeSet.toList().toVector());
}

QMultiHash<ModelNode, InformationName> NodeInstanceView::informationChanged(const QVector<InformationContainer> &containerVector)
{
    QMultiHash<ModelNode, InformationName> informationChangeHash;

    foreach (const InformationContainer &container, containerVector) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                InformationName informationChange = instance.setInformation(container.name(), container.information(), container.secondInformation(), container.thirdInformation());
                if (informationChange != NoInformationChange)
                    informationChangeHash.insert(instance.modelNode(), informationChange);
            }
        }
    }

    return informationChangeHash;
}

void NodeInstanceView::informationChanged(const InformationChangedCommand &command)
{
    if (!model())
        return;

    QMultiHash<ModelNode, InformationName> informationChangeHash = informationChanged(command.informations());

    if (!informationChangeHash.isEmpty())
        emitInstanceInformationsChange(informationChangeHash);
}

QImage NodeInstanceView::statePreviewImage(const ModelNode &stateNode) const
{
    if (stateNode == rootModelNode())
        return m_baseStatePreviewImage;

    return m_statePreviewImage.value(stateNode);
}

void NodeInstanceView::setPathToQt(const QString &pathToQt)
{
    if (m_pathToQt != pathToQt) {
        m_pathToQt = pathToQt;
        restartProcess();
    }
}

void NodeInstanceView::statePreviewImagesChanged(const StatePreviewImageChangedCommand &command)
{
    if (!model())
      return;

  QVector<ModelNode> previewImageChangeVector;

  foreach (const ImageContainer &container, command.previews()) {
      if (container.keyNumber() == -1) {
          m_baseStatePreviewImage = container.image();
          previewImageChangeVector.append(rootModelNode());
      } else if (hasInstanceForId(container.instanceId())) {
          ModelNode node = modelNodeForInternalId(container.instanceId());
          m_statePreviewImage.insert(node, container.image());
          previewImageChangeVector.append(node);
      }
  }

  if (!previewImageChangeVector.isEmpty())
       emitInstancesPreviewImageChanged(previewImageChangeVector);
}

void NodeInstanceView::componentCompleted(const ComponentCompletedCommand &command)
{
    if (!model())
        return;

    QVector<ModelNode> nodeVector;

    foreach (const qint32 &instanceId, command.instances()) {
        if (hasModelNodeForInternalId(instanceId))
            nodeVector.append(modelNodeForInternalId(instanceId));
    }

    if (!nodeVector.isEmpty())
        emitInstancesCompleted(nodeVector);
}

void NodeInstanceView::childrenChanged(const ChildrenChangedCommand &command)
{
     if (!model())
        return;


    QVector<ModelNode> childNodeVector;

    foreach (qint32 instanceId, command.childrenInstances()) {
        if (hasInstanceForId(instanceId)) {
            NodeInstance instance = instanceForId(instanceId);
            instance.setParentId(command.parentInstanceId());
            childNodeVector.append(instance.modelNode());
        }
    }

    QMultiHash<ModelNode, InformationName> informationChangeHash = informationChanged(command.informations());

    if (!informationChangeHash.isEmpty())
        emitInstanceInformationsChange(informationChangeHash);

    if (!childNodeVector.isEmpty())
        emitInstancesChildrenChanged(childNodeVector);
}

void NodeInstanceView::token(const TokenCommand &command)
{
    if (!model())
        return;

    QVector<ModelNode> nodeVector;

    foreach (const qint32 &instanceId, command.instances()) {
        if (hasModelNodeForInternalId(instanceId))
            nodeVector.append(modelNodeForInternalId(instanceId));
    }


    emitInstanceToken(command.tokenName(), command.tokenNumber(), nodeVector);
}

void NodeInstanceView::debugOutput(const DebugOutputCommand & /*command*/)
{
}

void NodeInstanceView::sendToken(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    QVector<qint32> instanceIdVector;
    foreach (const ModelNode &node, nodeVector)
        instanceIdVector.append(node.internalId());

    nodeInstanceServer()->token(TokenCommand(token, number, instanceIdVector));
}

QWidget *NodeInstanceView::widget()
{
    return 0;
}

}
