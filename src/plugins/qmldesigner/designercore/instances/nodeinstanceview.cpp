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

#include "nodeinstanceview.h"

#include <QUrl>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMultiHash>
#include <QTimerEvent>

#include <model.h>
#include <modelnode.h>
#include <metainfo.h>
#include <rewriterview.h>

#include "abstractproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "qmlchangeset.h"
#include "qmlstate.h"
#include "qmltimelinemutator.h"
#include "qmltimelinekeyframes.h"

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
#include "statepreviewimagechangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "tokencommand.h"
#include "removesharedmemorycommand.h"
#include "debugoutputcommand.h"

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
    \brief The NodeInstanceView class is the central class to create and manage
    instances of the ModelNode class.

    This view is used to instantiate the model nodes. Many abstract views hold a
    node instance view to get values from the node instances.
    For this purpose, this view can be rendered offscreen.

    \sa NodeInstance, ModelNode
*/

namespace QmlDesigner {

/*!
  Constructs a node instance view object as a child of \a parent. If \a parent
  is destructed, this instance is destructed, too.

  The class will be rendered offscreen if not set otherwise.

    \sa ~NodeInstanceView, setRenderOffScreen()
*/
NodeInstanceView::NodeInstanceView(QObject *parent, NodeInstanceServerInterface::RunModus runModus)
        : AbstractView(parent),
          m_baseStatePreviewImage(QSize(100, 100), QImage::Format_ARGB32),
          m_runModus(runModus),
          m_restartProcessTimerId(0)
{
    m_baseStatePreviewImage.fill(0xFFFFFF);
}


/*!
    Destructs a node instance view object.
*/
NodeInstanceView::~NodeInstanceView()
{
    removeAllInstanceNodeRelationships();
    delete nodeInstanceServer();
    m_currentKit = 0;
}

//\{

bool isSkippedRootNode(const ModelNode &node)
{
    static const PropertyNameList skipList({"Qt.ListModel", "QtQuick.ListModel", "Qt.ListModel", "QtQuick.ListModel"});

    if (skipList.contains(node.type()))
        return true;

    return false;
}


bool isSkippedNode(const ModelNode &node)
{
    static const PropertyNameList skipList({"QtQuick.XmlRole", "Qt.XmlRole", "QtQuick.ListElement", "Qt.ListElement"});

    if (skipList.contains(node.type()))
        return true;

    return false;
}

/*!
    Notifies the view that it was attached to \a model. For every model node in
    the model, a NodeInstance will be created.
*/

void NodeInstanceView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    auto server = new NodeInstanceServerProxy(this, m_runModus, m_currentKit, m_currentProject);
    m_nodeInstanceServer = server;
    m_lastCrashTime.start();
    connect(server, &NodeInstanceServerProxy::processCrashed, this, &NodeInstanceView::handleCrash);

    if (!isSkippedRootNode(rootModelNode()))
        nodeInstanceServer()->createScene(createCreateSceneCommand());

    ModelNode stateNode = currentStateNode();
    if (stateNode.isValid() && stateNode.metaInfo().isSubclassOf("QtQuick.State", 1, 0)) {
        NodeInstance newStateInstance = instanceForModelNode(stateNode);
        activateState(newStateInstance);
    }

}

void NodeInstanceView::modelAboutToBeDetached(Model * model)
{
    removeAllInstanceNodeRelationships();
    if (nodeInstanceServer()) {
        nodeInstanceServer()->clearScene(createClearSceneCommand());
        delete nodeInstanceServer();
    }
    m_statePreviewImage.clear();
    m_baseStatePreviewImage = QImage();
    removeAllInstanceNodeRelationships();
    m_activeStateInstance = NodeInstance();
    m_rootNodeInstance = NodeInstance();
    AbstractView::modelAboutToBeDetached(model);
}

void NodeInstanceView::handleCrash()
{
    int elaspsedTimeSinceLastCrash = m_lastCrashTime.restart();
    int forceRestartTime = 2000;
#ifdef QT_DEBUG
    forceRestartTime = 4000;
#endif
    if (elaspsedTimeSinceLastCrash > forceRestartTime)
        restartProcess();
    else
        emitDocumentMessage(tr("Qt Quick emulation layer crashed."));

    emitCustomNotification(QStringLiteral("puppet crashed"));
}

void NodeInstanceView::restartProcess()
{
    if (rootNodeInstance().isValid())
        rootNodeInstance().setError({});
    emitInstanceErrorChange({});
    emitDocumentMessage({}, {});

    if (m_restartProcessTimerId)
        killTimer(m_restartProcessTimerId);

    if (model()) {
        delete nodeInstanceServer();

        auto server = new NodeInstanceServerProxy(this, m_runModus, m_currentKit, m_currentProject);
        m_nodeInstanceServer = server;
        connect(server, &NodeInstanceServerProxy::processCrashed, this, &NodeInstanceView::handleCrash);

        if (!isSkippedRootNode(rootModelNode()))
            nodeInstanceServer()->createScene(createCreateSceneCommand());

        ModelNode stateNode = currentStateNode();
        if (stateNode.isValid() && stateNode.metaInfo().isSubclassOf("QtQuick.State", 1, 0)) {
            NodeInstance newStateInstance = instanceForModelNode(stateNode);
            activateState(newStateInstance);
        }
    }

    m_restartProcessTimerId = 0;
}

void NodeInstanceView::delayedRestartProcess()
{
    if (0 == m_restartProcessTimerId)
        m_restartProcessTimerId = startTimer(100);
}

void NodeInstanceView::nodeCreated(const ModelNode &createdNode)
{
    NodeInstance instance = loadNode(createdNode);

    if (isSkippedNode(createdNode))
        return;

    QList<VariantProperty> propertyList;
    propertyList.append(createdNode.variantProperty("x"));
    propertyList.append(createdNode.variantProperty("y"));
    updatePosition(propertyList);

    nodeInstanceServer()->createInstances(createCreateInstancesCommand({instance}));
    nodeInstanceServer()->changePropertyValues(createChangeValueCommand(createdNode.variantProperties()));
    nodeInstanceServer()->completeComponent(createComponentCompleteCommand({instance}));
}

/*! Notifies the view that \a removedNode will be removed.
*/
void NodeInstanceView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    nodeInstanceServer()->removeInstances(createRemoveInstancesCommand(removedNode));
    nodeInstanceServer()->removeSharedMemory(createRemoveSharedMemoryCommand("Image", removedNode.internalId()));
    removeInstanceAndSubInstances(removedNode);
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

    RemoveInstancesCommand removeInstancesCommand = createRemoveInstancesCommand(nodeList);

    if (!removeInstancesCommand.instanceIds().isEmpty())
        nodeInstanceServer()->removeInstances(removeInstancesCommand);

    nodeInstanceServer()->removeSharedMemory(createRemoveSharedMemoryCommand("Image", nodeList));
    nodeInstanceServer()->removeProperties(createRemovePropertiesCommand(nonNodePropertyList));

    foreach (const AbstractProperty &property, propertyList) {
        const PropertyName &name = property.name();
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

void NodeInstanceView::removeInstanceAndSubInstances(const ModelNode &node)
{
    foreach (const ModelNode &subNode, node.allSubModelNodes()) {
        if (hasInstanceForModelNode(subNode))
            removeInstanceNodeRelationship(subNode);
    }

    if (hasInstanceForModelNode(node))
        removeInstanceNodeRelationship(node);
}

void NodeInstanceView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    restartProcess();
}

void NodeInstanceView::nodeTypeChanged(const ModelNode &, const TypeName &, int, int)
{
    restartProcess();
}

void NodeInstanceView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    nodeInstanceServer()->changePropertyBindings(createChangeBindingCommand(propertyList));
}

/*!
    Notifies the view that abstract property values specified by \a propertyList
    were changed for a model node.

    The property will be set for the node instance.

    \sa AbstractProperty, NodeInstance, ModelNode
*/

void NodeInstanceView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    updatePosition(propertyList);
    nodeInstanceServer()->changePropertyValues(createChangeValueCommand(propertyList));
}
/*!
  Notifies the view that the property parent of the model node \a node has
  changed from \a oldPropertyParent to \a newPropertyParent.

  \note Also the \c {ModelNode::childNodes()} list was changed. The
  Node instance tree will be changed to reflect the model node tree change.

    \sa NodeInstance, ModelNode
*/

void NodeInstanceView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (!isSkippedNode(node)) {
        updateChildren(newPropertyParent);
        nodeInstanceServer()->reparentInstances(createReparentInstancesCommand(node, newPropertyParent, oldPropertyParent));
    }
}

void NodeInstanceView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &newUrl)
{
    nodeInstanceServer()->changeFileUrl(createChangeFileUrlCommand(newUrl));
}

void NodeInstanceView::nodeIdChanged(const ModelNode& node, const QString& /*newId*/, const QString& /*oldId*/)
{
    if (hasInstanceForModelNode(node)) {
        NodeInstance instance = instanceForModelNode(node);
        nodeInstanceServer()->changeIds(createChangeIdsCommand({instance}));
    }
}

void NodeInstanceView::nodeOrderChanged(const NodeListProperty & listProperty,
                                        const ModelNode & /*movedNode*/, int /*oldIndex*/)
{
    QVector<ReparentContainer> containerList;
    PropertyName propertyName = listProperty.name();
    qint32 containerInstanceId = -1;
    ModelNode containerNode = listProperty.parentModelNode();
    if (hasInstanceForModelNode(containerNode))
        containerInstanceId = instanceForModelNode(containerNode).instanceId();

    foreach (const ModelNode &node, listProperty.toModelNodeList()) {
        qint32 instanceId = -1;
        if (hasInstanceForModelNode(node)) {
            instanceId = instanceForModelNode(node).instanceId();
            ReparentContainer container(instanceId, containerInstanceId, propertyName, containerInstanceId, propertyName);
            containerList.append(container);
        }
    }

    nodeInstanceServer()->reparentInstances(ReparentInstancesCommand(containerList));
}

void NodeInstanceView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    restartProcess();
}

void NodeInstanceView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data)
{
    if ((node.isRootNode() && (name == "width" || name == "height")) || name.endsWith(PropertyName("@NodeInstance"))) {
        if (hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            QVariant value = data;
            if (value.isValid()) {
                PropertyValueContainer container(instance.instanceId(), name, value, TypeName());
                ChangeAuxiliaryCommand changeAuxiliaryCommand({container});
                nodeInstanceServer()->changeAuxiliaryValues(changeAuxiliaryCommand);
            } else {
                if (node.hasVariantProperty(name)) {
                    PropertyValueContainer container(instance.instanceId(), name, node.variantProperty(name).value(), TypeName());
                    ChangeValuesCommand changeValueCommand({container});
                    nodeInstanceServer()->changePropertyValues(changeValueCommand);
                } else if (node.hasBindingProperty(name)) {
                    PropertyBindingContainer container(instance.instanceId(), name, node.bindingProperty(name).expression(), TypeName());
                    ChangeBindingsCommand changeValueCommand({container});
                    nodeInstanceServer()->changePropertyBindings(changeValueCommand);
                }
            }
        }
    }
}

void NodeInstanceView::customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &, const QList<QVariant> &)
{
    if (view && identifier == QStringLiteral("reset QmlPuppet"))
        delayedRestartProcess();
}

void NodeInstanceView::nodeSourceChanged(const ModelNode &node, const QString & newNodeSource)
{
     if (hasInstanceForModelNode(node)) {
         NodeInstance instance = instanceForModelNode(node);
         ChangeNodeSourceCommand changeNodeSourceCommand(instance.instanceId(), newNodeSource);
         nodeInstanceServer()->changeNodeSource(changeNodeSourceCommand);
     }
}

void NodeInstanceView::currentStateChanged(const ModelNode &node)
{
    NodeInstance newStateInstance = instanceForModelNode(node);

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

/*!
    Returns a list of all node instances.

    \sa NodeInstance
*/

QList<NodeInstance> NodeInstanceView::instances() const
{
    return m_nodeInstanceHash.values();
}

/*!
    Returns the node instance for \a node, which must be valid.

    Returns an invalid node instance if no node instance for this model node
    exists.

    \sa NodeInstance
*/
NodeInstance NodeInstanceView::instanceForModelNode(const ModelNode &node) const
{
    Q_ASSERT(node.isValid());
    Q_ASSERT(m_nodeInstanceHash.contains(node));
    Q_ASSERT(m_nodeInstanceHash.value(node).modelNode() == node);
    return m_nodeInstanceHash.value(node);
}

bool NodeInstanceView::hasInstanceForModelNode(const ModelNode &node) const
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


/*!
    Returns the root node instance of this view.

    \sa NodeInstance
*/
NodeInstance NodeInstanceView::rootNodeInstance() const
{
    return m_rootNodeInstance;
}

/*!
    Returns the \a instance of this view.

  This can be the root node instance if it is specified in the QML file.
\code
    QGraphicsView {
         QGraphicsScene {
             Item {}
         }
    }
\endcode

    If there is node view in the QML file:
 \code

    Item {}

\endcode
    Then a new node instance for this QGraphicsView is
    generated which is not the root instance of this node instance view.

    This is the way to get this QGraphicsView node instance.

    \sa NodeInstance
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
    NodeInstance instance = instanceForModelNode(node);
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

void NodeInstanceView::updateChildren(const NodeAbstractProperty &newPropertyParent)
{
    QVector<ModelNode> childNodeVector = newPropertyParent.directSubNodes().toVector();

    qint32 parentInstanceId = newPropertyParent.parentModelNode().internalId();

    foreach (const ModelNode &childNode, childNodeVector) {
        qint32 instanceId = childNode.internalId();
        if (hasInstanceForId(instanceId)) {
            NodeInstance instance = instanceForId(instanceId);
            if (instance.directUpdates())
                instance.setParentId(parentInstanceId);
        }
    }

    if (!childNodeVector.isEmpty())
        emitInstancesChildrenChanged(childNodeVector);
}

void setXValue(NodeInstance &instance, const VariantProperty &variantProperty, QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    instance.setX(variantProperty.value().toDouble());
    informationChangeHash.insert(instance.modelNode(), Transform);
}

void setYValue(NodeInstance &instance, const VariantProperty &variantProperty, QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    instance.setY(variantProperty.value().toDouble());
    informationChangeHash.insert(instance.modelNode(), Transform);
}


void NodeInstanceView::updatePosition(const QList<VariantProperty> &propertyList)
{
    QMultiHash<ModelNode, InformationName> informationChangeHash;

    foreach (const VariantProperty &variantProperty, propertyList) {
        if (variantProperty.name() == "x") {
            const ModelNode modelNode = variantProperty.parentModelNode();
            if (!currentState().isBaseState() && QmlPropertyChanges::isValidQmlPropertyChanges(modelNode)) {
                ModelNode targetModelNode = QmlPropertyChanges(modelNode).target();
                if (targetModelNode.isValid()) {
                    NodeInstance instance = instanceForModelNode(targetModelNode);
                    setXValue(instance, variantProperty, informationChangeHash);
                }
            } else {
                NodeInstance instance = instanceForModelNode(modelNode);
                setXValue(instance, variantProperty, informationChangeHash);
            }
        } else if (variantProperty.name() == "y") {
            const ModelNode modelNode = variantProperty.parentModelNode();
            if (!currentState().isBaseState() && QmlPropertyChanges::isValidQmlPropertyChanges(modelNode)) {
                ModelNode targetModelNode = QmlPropertyChanges(modelNode).target();
                if (targetModelNode.isValid()) {
                    NodeInstance instance = instanceForModelNode(targetModelNode);
                    setYValue(instance, variantProperty, informationChangeHash);
                }
            } else {
                NodeInstance instance = instanceForModelNode(modelNode);
                setYValue(instance, variantProperty, informationChangeHash);
            }
        } else if (currentTimeline().isValid()
                   && variantProperty.name() == "value"
                   &&  QmlTimelineFrames::isValidKeyframe(variantProperty.parentModelNode())) {

            QmlTimelineFrames frames = QmlTimelineFrames::keyframesForKeyframe(variantProperty.parentModelNode());

            if (frames.isValid() && frames.propertyName() == "x" && frames.target().isValid()) {

                NodeInstance instance = instanceForModelNode(frames.target());
                setXValue(instance, variantProperty, informationChangeHash);
            } else if (frames.isValid() && frames.propertyName() == "y" && frames.target().isValid()) {
                NodeInstance instance = instanceForModelNode(frames.target());
                setYValue(instance, variantProperty, informationChangeHash);
            }

        }
    }

    if (!informationChangeHash.isEmpty())
        emitInstanceInformationsChange(informationChangeHash);
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

    foreach (const ModelNode &childNode, removedNode.directSubModelNodes())
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
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            QHashIterator<PropertyName, QVariant> auxiliaryIterator(node.auxiliaryData());
            while (auxiliaryIterator.hasNext()) {
                auxiliaryIterator.next();
                PropertyValueContainer container(instance.instanceId(), auxiliaryIterator.key(), auxiliaryIterator.value(), TypeName());
                auxiliaryContainerVector.append(container);
            }
        }
    }


    QVector<InstanceContainer> instanceContainerList;
    foreach (const NodeInstance &instance, instanceList) {
        InstanceContainer::NodeSourceType nodeSourceType = static_cast<InstanceContainer::NodeSourceType>(instance.modelNode().nodeSourceType());

        InstanceContainer::NodeMetaType nodeMetaType = InstanceContainer::ObjectMetaType;
        if (instance.modelNode().metaInfo().isSubclassOf("QtQuick.Item"))
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
            ReparentContainer container(instance.instanceId(), -1, PropertyName(), instanceForModelNode(parentProperty.parentModelNode()).instanceId(), parentProperty.name());
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
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyValueContainer container(instance.instanceId(), property.name(), property.value(), property.dynamicTypeName());
            valueContainerList.append(container);
        }
    }

    QVector<PropertyBindingContainer> bindingContainerList;
    foreach (const BindingProperty &property, bindingPropertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyBindingContainer container(instance.instanceId(), property.name(), property.expression(), property.dynamicTypeName());
            bindingContainerList.append(container);
        }
    }

    QVector<AddImportContainer> importVector;
    foreach (const Import &import, model()->imports())
        importVector.append(AddImportContainer(import.url(), import.file(), import.version(), import.alias(), import.importPaths()));

    QVector<MockupTypeContainer> mockupTypesVector;

    for (const CppTypeData &cppTypeData : model()->rewriterView()->getCppTypes()) {
        const QString versionString = cppTypeData.versionString;
        int majorVersion = -1;
        int minorVersion = -1;

        if (versionString.contains(QStringLiteral("."))) {
            const QStringList splittedString = versionString.split(QStringLiteral("."));
            majorVersion = splittedString.first().toInt();
            minorVersion = splittedString.last().toInt();
        }

        bool isItem = false;

        if (!cppTypeData.isSingleton) { /* Singletons only appear on the right hand sides of bindings and create just warnings. */
            const TypeName typeName = cppTypeData.typeName.toUtf8();
            const QString uri = cppTypeData.importUrl;

            NodeMetaInfo metaInfo = model()->metaInfo(uri.toUtf8() + "." + typeName);

            if (metaInfo.isValid())
                isItem = metaInfo.isGraphicalItem();

            MockupTypeContainer mockupType(typeName, uri, majorVersion, minorVersion, isItem);

            mockupTypesVector.append(mockupType);
        } else { /* We need a type for the signleton import */
            const TypeName typeName = cppTypeData.typeName.toUtf8() + "Mockup";
            const QString uri = cppTypeData.importUrl;

            MockupTypeContainer mockupType(typeName, uri, majorVersion, minorVersion, isItem);

            mockupTypesVector.append(mockupType);
        }
    }


    return CreateSceneCommand(instanceContainerList,
                              reparentContainerList,
                              idContainerList,
                              valueContainerList,
                              bindingContainerList,
                              auxiliaryContainerVector,
                              importVector,
                              mockupTypesVector,
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
        if (instance.modelNode().metaInfo().isSubclassOf("QtQuick.Item"))
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
            ReparentContainer container(instance.instanceId(), -1, PropertyName(), instanceForModelNode(parentProperty.parentModelNode()).instanceId(), parentProperty.name());
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

    if (newPropertyParent.isValid() && hasInstanceForModelNode(newPropertyParent.parentModelNode()))
        newParentInstanceId = instanceForModelNode(newPropertyParent.parentModelNode()).instanceId();


    if (oldPropertyParent.isValid() && hasInstanceForModelNode(oldPropertyParent.parentModelNode()))
        oldParentInstanceId = instanceForModelNode(oldPropertyParent.parentModelNode()).instanceId();


    ReparentContainer container(instanceForModelNode(node).instanceId(), oldParentInstanceId, oldPropertyParent.name(), newParentInstanceId, newPropertyParent.name());

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
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
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
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
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
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);

            if (instance.instanceId() >= 0)
                idList.append(instance.instanceId());
        }
    }

    return RemoveInstancesCommand(idList);
}

RemoveInstancesCommand NodeInstanceView::createRemoveInstancesCommand(const ModelNode &node) const
{
    QVector<qint32> idList;

    if (node.isValid() && hasInstanceForModelNode(node))
        idList.append(instanceForModelNode(node).instanceId());

    return RemoveInstancesCommand(idList);
}

RemovePropertiesCommand NodeInstanceView::createRemovePropertiesCommand(const QList<AbstractProperty> &propertyList) const
{
    QVector<PropertyAbstractContainer> containerList;

    foreach (const AbstractProperty &property, propertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyAbstractContainer container(instance.instanceId(), property.name(), property.dynamicTypeName());
            containerList.append(container);
        }

    }

    return RemovePropertiesCommand(containerList);
}

RemoveSharedMemoryCommand NodeInstanceView::createRemoveSharedMemoryCommand(const QString &sharedMemoryTypeName, quint32 keyNumber)
{
    return RemoveSharedMemoryCommand(sharedMemoryTypeName, {static_cast<qint32>(keyNumber)});
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

    QList<QPair<ModelNode, PropertyName> > valuePropertyChangeList;

    foreach (const PropertyValueContainer &container, command.valueChanges()) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instance.setProperty(container.name(), container.value());
                valuePropertyChangeList.append({instance.modelNode(), container.name()});
            }
        }
    }

    nodeInstanceServer()->removeSharedMemory(createRemoveSharedMemoryCommand(QStringLiteral("Values"), command.keyNumber()));

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

    m_nodeInstanceServer->benchmark(Q_FUNC_INFO + QString::number(renderImageChangeSet.count()));

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

    m_nodeInstanceServer->benchmark(Q_FUNC_INFO + QString::number(informationChangeHash.count()));

    if (!informationChangeHash.isEmpty())
        emitInstanceInformationsChange(informationChangeHash);
}

QImage NodeInstanceView::statePreviewImage(const ModelNode &stateNode) const
{
    if (stateNode == rootModelNode())
        return m_baseStatePreviewImage;

    return m_statePreviewImage.value(stateNode);
}

void NodeInstanceView::setKit(ProjectExplorer::Kit *newKit)
{
    if (m_currentKit != newKit) {
        m_currentKit = newKit;
        restartProcess();
    }
}

void NodeInstanceView::setProject(ProjectExplorer::Project *project)
{
    if (m_currentProject != project) {
        m_currentProject = project;
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
          if (!container.image().isNull())
              previewImageChangeVector.append(rootModelNode());
      } else if (hasInstanceForId(container.instanceId())) {
          ModelNode node = modelNodeForInternalId(container.instanceId());
          m_statePreviewImage.insert(node, container.image());
          if (!container.image().isNull())
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

    m_nodeInstanceServer->benchmark(Q_FUNC_INFO + QString::number(nodeVector.count()));

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
            if (instance.parentId() == -1 || !instance.directUpdates())
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

void NodeInstanceView::debugOutput(const DebugOutputCommand & command)
{
    DocumentMessage error(tr("Qt Quick emulation layer crashed."));
    if (command.instanceIds().isEmpty()) {
        emitDocumentMessage(command.text());
    } else {
        QVector<qint32> instanceIdsWithChangedErrors;
        foreach (qint32 instanceId, command.instanceIds()) {
            NodeInstance instance = instanceForId(instanceId);
            if (instance.isValid()) {
                if (instance.setError(command.text()))
                    instanceIdsWithChangedErrors.append(instanceId);
            } else {
                emitDocumentMessage(command.text());
            }
        }
        emitInstanceErrorChange(instanceIdsWithChangedErrors);
    }
}

void NodeInstanceView::sendToken(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    QVector<qint32> instanceIdVector;
    foreach (const ModelNode &node, nodeVector)
        instanceIdVector.append(node.internalId());

    nodeInstanceServer()->token(TokenCommand(token, number, instanceIdVector));
}

void NodeInstanceView::timerEvent(QTimerEvent *event)
{
    if (m_restartProcessTimerId == event->timerId())
        restartProcess();
}

}
