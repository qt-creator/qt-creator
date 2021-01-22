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

#include "abstractproperty.h"
#include "bindingproperty.h"
#include "captureddatacommand.h"
#include "changeauxiliarycommand.h"
#include "changebindingscommand.h"
#include "changefileurlcommand.h"
#include "changeidscommand.h"
#include "changelanguagecommand.h"
#include "changenodesourcecommand.h"
#include "changepreviewimagesizecommand.h"
#include "changeselectioncommand.h"
#include "changestatecommand.h"
#include "changevaluescommand.h"
#include "childrenchangedcommand.h"
#include "clearscenecommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "connectionmanagerinterface.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "debugoutputcommand.h"
#include "informationchangedcommand.h"
#include "inputeventcommand.h"
#include "nodeabstractproperty.h"
#include "nodeinstanceserverproxy.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "pixmapchangedcommand.h"
#include "puppettocreatorcommand.h"
#include "qmlchangeset.h"
#include "qmldesignerconstants.h"
#include "qmlstate.h"
#include "qmltimeline.h"
#include "qmltimelinekeyframegroup.h"
#include "qmlvisualnode.h"
#include "removeinstancescommand.h"
#include "removepropertiescommand.h"
#include "removesharedmemorycommand.h"
#include "reparentinstancescommand.h"
#include "scenecreatedcommand.h"
#include "statepreviewimagechangedcommand.h"
#include "tokencommand.h"
#include "update3dviewstatecommand.h"
#include "valueschangedcommand.h"
#include "variantproperty.h"
#include "view3dactioncommand.h"
#include "requestmodelnodepreviewimagecommand.h"

#include <designersettings.h>
#include <metainfo.h>
#include <model.h>
#include <modelnode.h>
#include <nodehints.h>
#include <rewriterview.h>
#include <qmlitemnode.h>

#ifndef QMLDESIGNER_TEST
#include <qmldesignerplugin.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <hdrimage.h>
#endif

#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlmultilanguageaspect.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QUrl>
#include <QMultiHash>
#include <QTimerEvent>
#include <QPicture>
#include <QPainter>
#include <QDirIterator>
#include <QFileSystemWatcher>

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
NodeInstanceView::NodeInstanceView(ConnectionManagerInterface &connectionManager)
    : m_connectionManager(connectionManager)
    , m_baseStatePreviewImage(QSize(100, 100), QImage::Format_ARGB32)
    , m_restartProcessTimerId(0)
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
{
    m_baseStatePreviewImage.fill(0xFFFFFF);

    // Interval > 0 is used for QFileSystemWatcher related timers to allow all notifications
    // related to a single event to be received before we act.
    m_resetTimer.setSingleShot(true);
    m_resetTimer.setInterval(100);
    QObject::connect(&m_resetTimer, &QTimer::timeout, [this] {
        resetPuppet();
    });
    m_updateWatcherTimer.setSingleShot(true);
    m_updateWatcherTimer.setInterval(100);
    QObject::connect(&m_updateWatcherTimer, &QTimer::timeout, [this] {
        for (const auto &path : qAsConst(m_pendingUpdateDirs))
            updateWatcher(path);
        m_pendingUpdateDirs.clear();
    });

    connect(m_fileSystemWatcher, &QFileSystemWatcher::directoryChanged,
            [this](const QString &path) {
        const QSet<QString> pendingDirs = m_pendingUpdateDirs;
        for (const auto &pendingPath : pendingDirs) {
            if (path.startsWith(pendingPath)) {
                // no need to add path, already handled by a pending parent path
                return;
            } else if (pendingPath.startsWith(path)) {
                // Parent path to a pending path added, remove the pending path
                m_pendingUpdateDirs.remove(pendingPath);
            }
        }
        m_pendingUpdateDirs.insert(path);
        m_updateWatcherTimer.start();

    });
    connect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, [this] {
        m_resetTimer.start();
    });
}


/*!
    Destructs a node instance view object.
*/
NodeInstanceView::~NodeInstanceView()
{
    removeAllInstanceNodeRelationships();
    m_currentTarget = nullptr;
}

//\{

bool static isSkippedRootNode(const ModelNode &node)
{
    static const PropertyNameList skipList({"Qt.ListModel", "QtQuick.ListModel", "Qt.ListModel", "QtQuick.ListModel"});

    if (skipList.contains(node.type()))
        return true;

    return false;
}


bool static isSkippedNode(const ModelNode &node)
{
    static const PropertyNameList skipList({"QtQuick.XmlRole", "Qt.XmlRole", "QtQuick.ListElement", "Qt.ListElement"});

    if (skipList.contains(node.type()))
        return true;

    return false;
}

bool static parentTakesOverRendering(const ModelNode &modelNode)
{
    if (!modelNode.isValid())
        return false;

    ModelNode currentNode = modelNode;

    while (currentNode.hasParentProperty()) {
        currentNode = currentNode.parentProperty().parentModelNode();
        if (NodeHints::fromModelNode(currentNode).takesOverRenderingOfChildren())
            return true;
    }

    return false;
}

/*!
    Notifies the view that it was attached to \a model. For every model node in
    the model, a NodeInstance will be created.
*/

void NodeInstanceView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    m_nodeInstanceServer = createNodeInstanceServerProxy();
    m_lastCrashTime.start();
    m_connectionManager.setCrashCallback(m_crashCallback);

    if (!isSkippedRootNode(rootModelNode())) {
        m_nodeInstanceServer->createScene(createCreateSceneCommand());
        m_nodeInstanceServer->changeSelection(createChangeSelectionCommand(model->selectedNodes(this)));
    }

    ModelNode stateNode = currentStateNode();
    if (stateNode.isValid() && stateNode.metaInfo().isSubclassOf("QtQuick.State", 1, 0)) {
        NodeInstance newStateInstance = instanceForModelNode(stateNode);
        activateState(newStateInstance);
    }

    updateWatcher({});
}

void NodeInstanceView::modelAboutToBeDetached(Model * model)
{
    m_connectionManager.setCrashCallback({});

    removeAllInstanceNodeRelationships();
    if (m_nodeInstanceServer) {
        m_nodeInstanceServer->clearScene(createClearSceneCommand());
        m_nodeInstanceServer.reset();
    }
    m_statePreviewImage.clear();
    m_baseStatePreviewImage = QImage();
    removeAllInstanceNodeRelationships();
    m_activeStateInstance = NodeInstance();
    m_rootNodeInstance = NodeInstance();
    AbstractView::modelAboutToBeDetached(model);
    m_resetTimer.stop();
    m_updateWatcherTimer.stop();
    m_pendingUpdateDirs.clear();
    m_fileSystemWatcher->removePaths(m_fileSystemWatcher->directories());
    m_fileSystemWatcher->removePaths(m_fileSystemWatcher->files());
}

void NodeInstanceView::handleCrash()
{
    qint64 elaspsedTimeSinceLastCrash = m_lastCrashTime.restart();
    qint64 forceRestartTime = 2000;
#ifdef QT_DEBUG
    forceRestartTime = 4000;
#endif
    if (elaspsedTimeSinceLastCrash > forceRestartTime)
        restartProcess();
    else
        emitDocumentMessage(tr("Qt Quick emulation layer crashed."));

    emitCustomNotification(QStringLiteral("puppet crashed"));
}

void NodeInstanceView::startPuppetTransaction()
{
    /* We assume no transaction is active. */
    QTC_ASSERT(!m_puppetTransaction.isValid(), return);
    m_puppetTransaction = beginRewriterTransaction("NodeInstanceView::PuppetTransaction");
}

void NodeInstanceView::endPuppetTransaction()
{
    /* We assume a transaction is active. */
    QTC_ASSERT(m_puppetTransaction.isValid(), return);

    /* Committing a transaction should not throw, but if there is
     * an issue with rewriting we should show an error message, instead
     * of simply crashing.
     */

    try {
        m_puppetTransaction.commit();
    } catch (Exception &e) {
        e.showException();
    }
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
        m_nodeInstanceServer.reset();
        m_nodeInstanceServer = createNodeInstanceServerProxy();

        if (!isSkippedRootNode(rootModelNode())) {
            m_nodeInstanceServer->createScene(createCreateSceneCommand());
            m_nodeInstanceServer->changeSelection(
                createChangeSelectionCommand(model()->selectedNodes(this)));
        }

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

    m_nodeInstanceServer->createInstances(createCreateInstancesCommand({instance}));
    m_nodeInstanceServer->changePropertyValues(
        createChangeValueCommand(createdNode.variantProperties()));
    m_nodeInstanceServer->completeComponent(createComponentCompleteCommand({instance}));
}

/*! Notifies the view that \a removedNode will be removed.
*/
void NodeInstanceView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    m_nodeInstanceServer->removeInstances(createRemoveInstancesCommand(removedNode));
    m_nodeInstanceServer->removeSharedMemory(
        createRemoveSharedMemoryCommand("Image", removedNode.internalId()));
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
        m_nodeInstanceServer->changePropertyValues(createChangeValueCommand(valueList));

    if (!bindingList.isEmpty())
        m_nodeInstanceServer->changePropertyBindings(createChangeBindingCommand(bindingList));
}

void NodeInstanceView::resetVerticalAnchors(const ModelNode &modelNode)
{
    QList<BindingProperty> bindingList;
    QList<VariantProperty> valueList;

    if (modelNode.hasBindingProperty("x"))
        bindingList.append(modelNode.bindingProperty("x"));
    else if (modelNode.hasVariantProperty("y"))
        valueList.append(modelNode.variantProperty("y"));

    if (modelNode.hasBindingProperty("height"))
        bindingList.append(modelNode.bindingProperty("height"));
    else if (modelNode.hasVariantProperty("height"))
        valueList.append(modelNode.variantProperty("height"));

    if (!valueList.isEmpty())
        m_nodeInstanceServer->changePropertyValues(createChangeValueCommand(valueList));

    if (!bindingList.isEmpty())
        m_nodeInstanceServer->changePropertyBindings(createChangeBindingCommand(bindingList));
}

void NodeInstanceView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    QTC_ASSERT(m_nodeInstanceServer, return);

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
        m_nodeInstanceServer->removeInstances(removeInstancesCommand);

    m_nodeInstanceServer->removeSharedMemory(createRemoveSharedMemoryCommand("Image", nodeList));
    m_nodeInstanceServer->removeProperties(createRemovePropertiesCommand(nonNodePropertyList));

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
        } else if (name == "shader" && property.parentModelNode().isSubclassOf("QtQuick3D.Shader")) {
            m_resetTimer.start();
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
    QTC_ASSERT(m_nodeInstanceServer, return);
    m_nodeInstanceServer->changePropertyBindings(createChangeBindingCommand(propertyList));
}

/*!
    Notifies the view that abstract property values specified by \a propertyList
    were changed for a model node.

    The property will be set for the node instance.

    \sa AbstractProperty, NodeInstance, ModelNode
*/

void NodeInstanceView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    QTC_ASSERT(m_nodeInstanceServer, return);
    updatePosition(propertyList);
    m_nodeInstanceServer->changePropertyValues(createChangeValueCommand(propertyList));

    for (const auto &property : propertyList) {
        if (property.name() == "shader" && property.parentModelNode().isSubclassOf("QtQuick3D.Shader")) {
            m_resetTimer.start();
            break;
        }
    }
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
    QTC_ASSERT(m_nodeInstanceServer, return);
    if (!isSkippedNode(node)) {
        updateChildren(newPropertyParent);
        m_nodeInstanceServer->reparentInstances(
            createReparentInstancesCommand(node, newPropertyParent, oldPropertyParent));
    }
}

void NodeInstanceView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &newUrl)
{
    QTC_ASSERT(m_nodeInstanceServer, return);
    m_nodeInstanceServer->changeFileUrl(createChangeFileUrlCommand(newUrl));
}

void NodeInstanceView::nodeIdChanged(const ModelNode& node, const QString& /*newId*/, const QString &oldId)
{
    QTC_ASSERT(m_nodeInstanceServer, return);

    if (hasInstanceForModelNode(node)) {
        NodeInstance instance = instanceForModelNode(node);
        m_nodeInstanceServer->changeIds(createChangeIdsCommand({instance}));
        m_imageDataMap.remove(oldId);
    }
}

void NodeInstanceView::nodeOrderChanged(const NodeListProperty & listProperty,
                                        const ModelNode & /*movedNode*/, int /*oldIndex*/)
{
    QTC_ASSERT(m_nodeInstanceServer, return);
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

    m_nodeInstanceServer->reparentInstances(ReparentInstancesCommand(containerList));
}

void NodeInstanceView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    restartProcess();
}

void NodeInstanceView::auxiliaryDataChanged(const ModelNode &node,
                                            const PropertyName &name,
                                            const QVariant &value)
{
    QTC_ASSERT(m_nodeInstanceServer, return);
    if (((node.isRootNode() && (name == "width" || name == "height")) || name == "invisible" || name == "locked")
            || name.endsWith(PropertyName("@NodeInstance"))) {
        if (hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            if (value.isValid() || name == "invisible" || name == "locked") {
                PropertyValueContainer container{instance.instanceId(), name, value, TypeName()};
                m_nodeInstanceServer->changeAuxiliaryValues({{container}});
            } else {
                if (node.hasVariantProperty(name)) {
                    PropertyValueContainer container(instance.instanceId(), name, node.variantProperty(name).value(), TypeName());
                    ChangeValuesCommand changeValueCommand({container});
                    m_nodeInstanceServer->changePropertyValues(changeValueCommand);
                } else if (node.hasBindingProperty(name)) {
                    PropertyBindingContainer container{instance.instanceId(), name, node.bindingProperty(name).expression(), TypeName()};
                    m_nodeInstanceServer->changePropertyBindings({{container}});
                }
            }
        }
    } else if (node.isRootNode() && name == "language@Internal") {
        const QString languageAsString = value.toString();
        if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current(m_currentTarget))
            multiLanguageAspect->setCurrentLocale(languageAsString);
        m_nodeInstanceServer->changeLanguage({languageAsString});
    } else if (node.isRootNode() && name == "previewSize@Internal") {
        m_nodeInstanceServer->changePreviewImageSize(value.toSize());
    }
}

void NodeInstanceView::customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &, const QList<QVariant> &)
{
    if (view && identifier == QStringLiteral("reset QmlPuppet"))
        delayedRestartProcess();
}

void NodeInstanceView::nodeSourceChanged(const ModelNode &node, const QString & newNodeSource)
{
     QTC_ASSERT(m_nodeInstanceServer, return);
     if (hasInstanceForModelNode(node)) {
         NodeInstance instance = instanceForModelNode(node);
         ChangeNodeSourceCommand changeNodeSourceCommand(instance.instanceId(), newNodeSource);
         m_nodeInstanceServer->changeNodeSource(changeNodeSourceCommand);
     }
}

void NodeInstanceView::capturedData(const CapturedDataCommand &) {}

void NodeInstanceView::currentStateChanged(const ModelNode &node)
{
    NodeInstance newStateInstance = instanceForModelNode(node);

    if (newStateInstance.isValid() && node.metaInfo().isSubclassOf("QtQuick.State", 1, 0))
        nodeInstanceView()->activateState(newStateInstance);
    else
        nodeInstanceView()->activateBaseState();
}

void NodeInstanceView::sceneCreated(const SceneCreatedCommand &) {}

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
                   &&  QmlTimelineKeyframeGroup::isValidKeyframe(variantProperty.parentModelNode())) {

            QmlTimelineKeyframeGroup frames = QmlTimelineKeyframeGroup::keyframeGroupForKeyframe(variantProperty.parentModelNode());

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
    m_nodeInstanceServer->changeState(ChangeStateCommand(instance.instanceId()));
}

void NodeInstanceView::activateBaseState()
{
    m_nodeInstanceServer->changeState(ChangeStateCommand(-1));
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

    return {};
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

    for (const ModelNode &node : std::as_const(nodeList)) {
        NodeInstance instance = loadNode(node);
        if (!isSkippedNode(node))
            instanceList.append(instance);
    }

    nodeList = filterNodesForSkipItems(nodeList);

    QList<VariantProperty> variantPropertyList;
    QList<BindingProperty> bindingPropertyList;

    QVector<PropertyValueContainer> auxiliaryContainerVector;
    for (const ModelNode &node : std::as_const(nodeList)) {
        variantPropertyList.append(node.variantProperties());
        bindingPropertyList.append(node.bindingProperties());
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            const QHash<PropertyName, QVariant> aux = node.auxiliaryData();
            for (auto auxiliaryIterator = aux.cbegin(), end = aux.cend();
                      auxiliaryIterator != end;
                      ++auxiliaryIterator) {
                PropertyValueContainer container(instance.instanceId(), auxiliaryIterator.key(), auxiliaryIterator.value(), TypeName());
                auxiliaryContainerVector.append(container);
            }
        }
    }

    QVector<InstanceContainer> instanceContainerList;
    for (const NodeInstance &instance : std::as_const(instanceList)) {
        InstanceContainer::NodeSourceType nodeSourceType = static_cast<InstanceContainer::NodeSourceType>(instance.modelNode().nodeSourceType());

        InstanceContainer::NodeMetaType nodeMetaType = InstanceContainer::ObjectMetaType;
        if (instance.modelNode().metaInfo().isSubclassOf("QtQuick.Item"))
            nodeMetaType = InstanceContainer::ItemMetaType;

        InstanceContainer::NodeFlags nodeFlags;

        if (parentTakesOverRendering(instance.modelNode()))
            nodeFlags |= InstanceContainer::ParentTakesOverRendering;

        InstanceContainer container(instance.instanceId(),
                                    instance.modelNode().type(),
                                    instance.modelNode().majorVersion(),
                                    instance.modelNode().minorVersion(),
                                    instance.modelNode().metaInfo().componentFileName(),
                                    instance.modelNode().nodeSource(),
                                    nodeSourceType,
                                    nodeMetaType,
                                    nodeFlags);

        instanceContainerList.append(container);
    }

    QVector<ReparentContainer> reparentContainerList;
    for (const NodeInstance &instance : std::as_const(instanceList)) {
        if (instance.modelNode().hasParentProperty()) {
            NodeAbstractProperty parentProperty = instance.modelNode().parentProperty();
            ReparentContainer container(instance.instanceId(), -1, PropertyName(), instanceForModelNode(parentProperty.parentModelNode()).instanceId(), parentProperty.name());
            reparentContainerList.append(container);
        }
    }

    QVector<IdContainer> idContainerList;
    for (const NodeInstance &instance : std::as_const(instanceList)) {
        QString id = instance.modelNode().id();
        if (!id.isEmpty()) {
            IdContainer container(instance.instanceId(), id);
            idContainerList.append(container);
        }
    }

    QVector<PropertyValueContainer> valueContainerList;
    for (const VariantProperty &property : std::as_const(variantPropertyList)) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyValueContainer container(instance.instanceId(), property.name(), property.value(), property.dynamicTypeName());
            valueContainerList.append(container);
        }
    }

    QVector<PropertyBindingContainer> bindingContainerList;
    for (const BindingProperty &property : std::as_const(bindingPropertyList)) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyBindingContainer container(instance.instanceId(), property.name(), property.expression(), property.dynamicTypeName());
            bindingContainerList.append(container);
        }
    }

    QVector<AddImportContainer> importVector;
    for (const Import &import : model()->imports())
        importVector.append(AddImportContainer(import.url(), import.file(), import.version(), import.alias(), import.importPaths()));

    QVector<MockupTypeContainer> mockupTypesVector;

    for (const QmlTypeData &cppTypeData : model()->rewriterView()->getQMLTypes()) {
        const QString versionString = cppTypeData.versionString;
        int majorVersion = -1;
        int minorVersion = -1;

        if (versionString.contains(QStringLiteral("."))) {
            const QStringList splittedString = versionString.split(QStringLiteral("."));
            majorVersion = splittedString.constFirst().toInt();
            minorVersion = splittedString.constLast().toInt();
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

    QString lastUsedLanguage;
    if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current(m_currentTarget))
        lastUsedLanguage = multiLanguageAspect->currentLocale();

    ModelNode stateNode = currentStateNode();
    qint32 stateInstanceId = 0;
    if (stateNode.isValid() && stateNode.metaInfo().isSubclassOf("QtQuick.State", 1, 0))
        stateInstanceId = stateNode.internalId();

    return CreateSceneCommand(instanceContainerList,
                              reparentContainerList,
                              idContainerList,
                              valueContainerList,
                              bindingContainerList,
                              auxiliaryContainerVector,
                              importVector,
                              mockupTypesVector,
                              model()->fileUrl(),
#ifndef QMLDESIGNER_TEST
                              QUrl::fromLocalFile(QmlDesigner::DocumentManager::currentResourcePath()
                                                  .toFileInfo().absoluteFilePath()),
#else
                              QUrl::fromLocalFile(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath()),
#endif
                              m_edit3DToolStates[model()->fileUrl()],
                              lastUsedLanguage,
                              stateInstanceId);
}

ClearSceneCommand NodeInstanceView::createClearSceneCommand() const
{
    return {};
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

        InstanceContainer::NodeFlags nodeFlags;

        if (parentTakesOverRendering(instance.modelNode()))
            nodeFlags |= InstanceContainer::ParentTakesOverRendering;

        InstanceContainer container(instance.instanceId(),
                                    instance.modelNode().type(),
                                    instance.modelNode().majorVersion(),
                                    instance.modelNode().minorVersion(),
                                    instance.modelNode().metaInfo().componentFileName(),
                                    instance.modelNode().nodeSource(),
                                    nodeSourceType,
                                    nodeMetaType,
                                    nodeFlags);
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
    return {fileUrl};
}

ChangeValuesCommand NodeInstanceView::createChangeValueCommand(const QList<VariantProperty>& propertyList) const
{
    QVector<PropertyValueContainer> containerList;

    const bool reflectionFlag = m_puppetTransaction.isValid() && (!currentTimeline().isValid() || !currentTimeline().isRecording());

    foreach (const VariantProperty &property, propertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyValueContainer container(instance.instanceId(), property.name(), property.value(), property.dynamicTypeName());
            container.setReflectionFlag(reflectionFlag);
            containerList.append(container);
        }

    }

    return ChangeValuesCommand(containerList);
}

ChangeBindingsCommand NodeInstanceView::createChangeBindingCommand(const QList<BindingProperty> &propertyList) const
{
    QVector<PropertyBindingContainer> containerList;

    for (const BindingProperty &property : propertyList) {
        ModelNode node = property.parentModelNode();
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyBindingContainer container(instance.instanceId(), property.name(), property.expression(), property.dynamicTypeName());
            containerList.append(container);
        }

    }

    return {containerList};
}

ChangeIdsCommand NodeInstanceView::createChangeIdsCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<IdContainer> containerList;
    for (const NodeInstance &instance : instanceList) {
        QString id = instance.modelNode().id();
        if (!id.isEmpty()) {
            IdContainer container(instance.instanceId(), id);
            containerList.append(container);
        }
    }

    return {containerList};
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

ChangeSelectionCommand NodeInstanceView::createChangeSelectionCommand(const QList<ModelNode> &nodeList) const
{
    QVector<qint32> idList;
    foreach (const ModelNode &node, nodeList) {
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);

            if (instance.instanceId() >= 0)
                idList.append(instance.instanceId());
        }
    }

    return ChangeSelectionCommand(idList);
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

    m_nodeInstanceServer->removeSharedMemory(
        createRemoveSharedMemoryCommand(QStringLiteral("Values"), command.keyNumber()));

    if (!valuePropertyChangeList.isEmpty())
        emitInstancePropertyChange(valuePropertyChangeList);
}

void NodeInstanceView::valuesModified(const ValuesModifiedCommand &command)
{
    if (!model())
        return;

    if (command.transactionOption == ValuesModifiedCommand::TransactionOption::Start)
        startPuppetTransaction();

    for (const PropertyValueContainer &container : command.valueChanges()) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                // QmlVisualNode is needed so timeline and state are updated
                QmlVisualNode node = instance.modelNode();
                if (node.modelValue(container.name()) != container.value())
                    node.setVariantProperty(container.name(), container.value());
            }
        }
    }

    if (command.transactionOption == ValuesModifiedCommand::TransactionOption::End)
        endPuppetTransaction();
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
        emitInstancesRenderImageChanged(Utils::toList(renderImageChangeSet).toVector());
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

void NodeInstanceView::setTarget(ProjectExplorer::Target *newTarget)
{
    if (m_currentTarget != newTarget) {
        m_currentTarget = newTarget;
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

    m_nodeInstanceServer->token(TokenCommand(token, number, instanceIdVector));
}

void NodeInstanceView::selectionChanged(const ChangeSelectionCommand &command)
{
    clearSelectedModelNodes();
    foreach (const qint32 &instanceId, command.instanceIds()) {
        if (hasModelNodeForInternalId(instanceId))
            selectModelNode(modelNodeForInternalId(instanceId));
    }
}

void NodeInstanceView::handlePuppetToCreatorCommand(const PuppetToCreatorCommand &command)
{
    if (command.type() == PuppetToCreatorCommand::Edit3DToolState) {
        if (m_nodeInstanceServer) {
            auto data = qvariant_cast<QVariantList>(command.data());
            if (data.size() == 3) {
                QString qmlId = data[0].toString();
                m_edit3DToolStates[model()->fileUrl()][qmlId].insert(data[1].toString(), data[2]);
            }
        }
    } else if (command.type() == PuppetToCreatorCommand::Render3DView) {
        ImageContainer container = qvariant_cast<ImageContainer>(command.data());
        if (!container.image().isNull())
            emitRenderImage3DChanged(container.image());
    } else if (command.type() == PuppetToCreatorCommand::ActiveSceneChanged) {
        const auto sceneState = qvariant_cast<QVariantMap>(command.data());
        emitUpdateActiveScene3D(sceneState);
    } else if (command.type() == PuppetToCreatorCommand::RenderModelNodePreviewImage) {
        ImageContainer container = qvariant_cast<ImageContainer>(command.data());
        QImage image = container.image();
        if (hasModelNodeForInternalId(container.instanceId()) && !image.isNull()) {
            auto node = modelNodeForInternalId(container.instanceId());
            if (node.isValid()) {
#ifndef QMLDESIGNER_TEST
                const double ratio = QmlDesignerPlugin::formEditorDevicePixelRatio();
#else
                const double ratio = 1;
#endif
                const int dim = Constants::MODELNODE_PREVIEW_IMAGE_DIMENSIONS * ratio;
                if (image.height() != dim || image.width() != dim)
                    image = image.scaled(dim, dim, Qt::KeepAspectRatio);
                image.setDevicePixelRatio(ratio);
                updatePreviewImageForNode(node, image);
            }
        }
    } else if (command.type() == PuppetToCreatorCommand::Import3DSupport) {
        const QVariantMap supportMap = qvariant_cast<QVariantMap>(command.data());
        emitImport3DSupportChanged(supportMap);
    }
}

std::unique_ptr<NodeInstanceServerProxy> NodeInstanceView::createNodeInstanceServerProxy()
{
    return std::make_unique<NodeInstanceServerProxy>(this, m_currentTarget, m_connectionManager);
}

void NodeInstanceView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                            const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    m_nodeInstanceServer->changeSelection(createChangeSelectionCommand(selectedNodeList));
}

void NodeInstanceView::sendInputEvent(QInputEvent *e) const
{
    m_nodeInstanceServer->inputEvent(InputEventCommand(e));
}

void NodeInstanceView::view3DAction(const View3DActionCommand &command)
{
    m_nodeInstanceServer->view3DAction(command);
}

void NodeInstanceView::requestModelNodePreviewImage(const ModelNode &node, const ModelNode &renderNode)
{
    if (node.isValid()) {
        auto instance = instanceForModelNode(node);
        if (instance.isValid()) {
            qint32 renderItemId = -1;
            QString componentPath;
            if (renderNode.isValid()) {
                auto renderInstance = instanceForModelNode(renderNode);
                if (renderInstance.isValid())
                    renderItemId = renderInstance.instanceId();
                if (renderNode.isComponent())
                    componentPath = renderNode.metaInfo().componentFileName();
            } else if (node.isComponent()) {
                componentPath = node.metaInfo().componentFileName();
            }
#ifndef QMLDESIGNER_TEST
                const double ratio = QmlDesignerPlugin::formEditorDevicePixelRatio();
#else
                const double ratio = 1;
#endif
            const int dim = Constants::MODELNODE_PREVIEW_IMAGE_DIMENSIONS * ratio;
            m_nodeInstanceServer->requestModelNodePreviewImage(
                        RequestModelNodePreviewImageCommand(instance.instanceId(), QSize(dim, dim),
                                                            componentPath, renderItemId));
        }
    }
}

void NodeInstanceView::edit3DViewResized(const QSize &size) const
{
    m_nodeInstanceServer->update3DViewState(Update3dViewStateCommand(size));
}

void NodeInstanceView::timerEvent(QTimerEvent *event)
{
    if (m_restartProcessTimerId == event->timerId())
        restartProcess();
}

QVariant NodeInstanceView::modelNodePreviewImageDataToVariant(const ModelNodePreviewImageData &imageData)
{
    static const QPixmap placeHolder(":/navigator/icon/tooltip_placeholder.png");

    QVariantMap map;
    map.insert("type", imageData.type);
    if (imageData.pixmap.isNull())
        map.insert("pixmap", placeHolder);
    else
        map.insert("pixmap", QVariant::fromValue<QPixmap>(imageData.pixmap));
    map.insert("id", imageData.id);
    map.insert("info", imageData.info);
    return map;
}

QVariant NodeInstanceView::previewImageDataForImageNode(const ModelNode &modelNode)
{
    if (!modelNode.isValid())
        return {};

    VariantProperty prop = modelNode.variantProperty("source");
    QString imageSource = prop.value().toString();

    ModelNodePreviewImageData imageData;
    imageData.id = modelNode.id();
    imageData.type = QString::fromLatin1(modelNode.type());
#ifndef QMLDESIGNER_TEST
                const double ratio = QmlDesignerPlugin::formEditorDevicePixelRatio();
#else
                const double ratio = 1;
#endif

    if (imageSource.isEmpty() && modelNode.isSubclassOf("QtQuick3D.Texture")) {
        // Texture node may have sourceItem instead
        BindingProperty binding = modelNode.bindingProperty("sourceItem");
        if (binding.isValid()) {
            ModelNode boundNode = binding.resolveToModelNode();
            if (boundNode.isValid()) {
                // If bound node is a component, fall back to component render mechanism, as
                // QmlItemNode::instanceRenderPixmap() often includes unnecessary empty space
                // for those
                if (boundNode.isComponent()) {
                    return previewImageDataForGenericNode(modelNode, boundNode);
                } else {
                    QmlItemNode itemNode(boundNode);
                    const int dim = Constants::MODELNODE_PREVIEW_IMAGE_DIMENSIONS * ratio;
                    imageData.pixmap = itemNode.instanceRenderPixmap().scaled(dim, dim, Qt::KeepAspectRatio);
                    imageData.pixmap.setDevicePixelRatio(ratio);

                }
                imageData.info = QObject::tr("Source item: %1").arg(boundNode.id());
            }
        }
    } else {
        if (imageSource.isEmpty() && modelNode.isComponent()) {
            // Image component has no custom source set, so fall back to component rendering to get
            // the default component image.
            return previewImageDataForGenericNode(modelNode, {});
        }

        QFileInfo imageFi(imageSource);
        if (imageFi.isRelative())
            imageSource = QFileInfo(modelNode.model()->fileUrl().toLocalFile()).dir().absoluteFilePath(imageSource);

        imageFi = QFileInfo(imageSource);
        QDateTime modified = imageFi.lastModified();

        bool reload = true;
        if (m_imageDataMap.contains(imageData.id)) {
            imageData = m_imageDataMap[imageData.id];
            if (modified == imageData.time)
                reload = false;
        }

        if (reload) {
            QPixmap originalPixmap;
            if (modelNode.isSubclassOf("Qt.SafeRenderer.SafeRendererPicture")) {
                QPicture picture;
                picture.load(imageSource);
                if (!picture.isNull()) {
                    QImage paintImage(picture.width(), picture.height(), QImage::Format_ARGB32);
                    paintImage.fill(Qt::transparent);
                    QPainter painter(&paintImage);
                    painter.drawPicture(0, 0, picture);
                    painter.end();
                    originalPixmap = QPixmap::fromImage(paintImage);
                }
            } else {
#ifndef QMLDESIGNER_TEST
                if (imageFi.suffix() == "hdr")
                    originalPixmap = HdrImage{imageSource}.toPixmap();
                else
#endif
                    originalPixmap.load(imageSource);
            }
            if (!originalPixmap.isNull()) {
                const int dim = Constants::MODELNODE_PREVIEW_IMAGE_DIMENSIONS * ratio;
                imageData.pixmap = originalPixmap.scaled(dim, dim, Qt::KeepAspectRatio);
                imageData.pixmap.setDevicePixelRatio(ratio);

                double imgSize = double(imageFi.size());
                static QStringList units({QObject::tr("B"), QObject::tr("KB"), QObject::tr("MB"), QObject::tr("GB")});
                int unitIndex = 0;
                while (imgSize > 1024. && unitIndex < units.size() - 1) {
                    ++unitIndex;
                    imgSize /= 1024.;
                }
                imageData.info = QStringLiteral("%1 x %2\n%3%4 (%5)").arg(originalPixmap.width()).arg(originalPixmap.height())
                        .arg(QString::number(imgSize, 'g', 3)).arg(units[unitIndex]).arg(imageFi.suffix());
                m_imageDataMap.insert(imageData.id, imageData);
            }
        }
    }

    return modelNodePreviewImageDataToVariant(imageData);
}

QVariant NodeInstanceView::previewImageDataForGenericNode(const ModelNode &modelNode, const ModelNode &renderNode)
{
    ModelNodePreviewImageData imageData;

    // We need puppet to generate the image, which needs to be asynchronous.
    // Until the image is ready, we show a placeholder
    const QString id = modelNode.id();
    if (m_imageDataMap.contains(id)) {
        imageData = m_imageDataMap[id];
    } else {
        imageData.type = QString::fromLatin1(modelNode.type());
        imageData.id = id;
        m_imageDataMap.insert(id, imageData);
    }
    requestModelNodePreviewImage(modelNode, renderNode);

    return modelNodePreviewImageDataToVariant(imageData);
}

void NodeInstanceView::updatePreviewImageForNode(const ModelNode &modelNode, const QImage &image)
{
    QPixmap pixmap = QPixmap::fromImage(image);
    if (m_imageDataMap.contains(modelNode.id()))
        m_imageDataMap[modelNode.id()].pixmap = pixmap;
    emitModelNodelPreviewPixmapChanged(modelNode, pixmap);
}

void NodeInstanceView::updateWatcher(const QString &path)
{
    QString rootPath;
    QStringList oldFiles;
    QStringList oldDirs;
    QStringList newFiles;
    QStringList newDirs;

    if (path.isEmpty()) {
        // Do full update
        rootPath = QFileInfo(model()->fileUrl().toLocalFile()).absolutePath();
        m_fileSystemWatcher->removePaths(m_fileSystemWatcher->directories());
        m_fileSystemWatcher->removePaths(m_fileSystemWatcher->files());
    } else {
        rootPath = path;
        const QStringList files = m_fileSystemWatcher->files();
        const QStringList dirs = m_fileSystemWatcher->directories();
        for (const auto &file : files) {
            if (file.startsWith(path))
                oldFiles.append(file);
        }
        for (const auto &dir : dirs) {
            if (dir.startsWith(path))
                oldDirs.append(dir);
        }
    }

    newDirs.append(rootPath);

    QDirIterator dirIterator(rootPath, {}, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIterator.hasNext())
        newDirs.append(dirIterator.next());

    // Common shader suffixes
    static const QStringList filterList {"*.frag", "*.vert",
                                         "*.glsl", "*.glslv", "*.glslf",
                                         "*.vsh","*.fsh"};

    QDirIterator fileIterator(rootPath, filterList, QDir::Files, QDirIterator::Subdirectories);
    while (fileIterator.hasNext())
        newFiles.append(fileIterator.next());

    if (oldDirs != newDirs) {
        if (!oldDirs.isEmpty())
            m_fileSystemWatcher->removePaths(oldDirs);
        if (!newDirs.isEmpty())
            m_fileSystemWatcher->addPaths(newDirs);
    }

    if (newFiles != oldFiles) {
        if (!oldFiles.isEmpty())
            m_fileSystemWatcher->removePaths(oldFiles);
        if (!newFiles.isEmpty())
            m_fileSystemWatcher->addPaths(newFiles);
    }
}

}
