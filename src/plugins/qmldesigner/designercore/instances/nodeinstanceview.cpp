// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeinstanceview.h"

#include <abstractproperty.h>
#include <bindingproperty.h>
#include <captureddatacommand.h>
#include <changeauxiliarycommand.h>
#include <changebindingscommand.h>
#include <changefileurlcommand.h>
#include <changeidscommand.h>
#include <changelanguagecommand.h>
#include <changenodesourcecommand.h>
#include <changepreviewimagesizecommand.h>
#include <changeselectioncommand.h>
#include <changestatecommand.h>
#include <changevaluescommand.h>
#include <childrenchangedcommand.h>
#include <clearscenecommand.h>
#include <completecomponentcommand.h>
#include <componentcompletedcommand.h>
#include <connectionmanagerinterface.h>
#include <createinstancescommand.h>
#include <createscenecommand.h>
#include <debugoutputcommand.h>
#include <imageutils.h>
#include <informationchangedcommand.h>
#include <inputeventcommand.h>
#include <nanotrace/nanotrace.h>
#include <nanotracecommand.h>
#include <nodeabstractproperty.h>
#include <nodeinstanceserverproxy.h>
#include <nodelistproperty.h>
#include <pixmapchangedcommand.h>
#include <puppettocreatorcommand.h>
#include <qml3dnode.h>
#include <qmlchangeset.h>
#include <qmldesignerconstants.h>
#include <qmlstate.h>
#include <qmltimeline.h>
#include <qmltimelinekeyframegroup.h>
#include <qmlvisualnode.h>
#include <removeinstancescommand.h>
#include <removepropertiescommand.h>
#include <removesharedmemorycommand.h>
#include <reparentinstancescommand.h>
#include <requestmodelnodepreviewimagecommand.h>
#include <scenecreatedcommand.h>
#include <statepreviewimagechangedcommand.h>
#include <tokencommand.h>
#include <update3dviewstatecommand.h>
#include <valueschangedcommand.h>
#include <variantproperty.h>
#include <view3dactioncommand.h>

#include <auxiliarydataproperties.h>
#include <designersettings.h>
#include <externaldependenciesinterface.h>
#include <metainfo.h>
#include <model.h>
#include <model/modelutils.h>
#include <modelnode.h>
#include <nodehints.h>
#include <qmlitemnode.h>
#include <rewriterview.h>

#include <utils/hdrimage.h>

#include <coreplugin/messagemanager.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlmultilanguageaspect.h>
#include <qmlprojectmanager/qmlproject.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/threadutils.h>

#include <qtsupport/qtkitaspect.h>

#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QImageReader>
#include <QLocale>
#include <QMultiHash>
#include <QPainter>
#include <QPicture>
#include <QScopedPointer>
#include <QTimerEvent>
#include <QUrl>

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
NodeInstanceView::NodeInstanceView(ConnectionManagerInterface &connectionManager,
                                   ExternalDependenciesInterface &externalDependencies,
                                   bool qsbEnabled)
    : AbstractView{externalDependencies}
    , m_connectionManager(connectionManager)
    , m_externalDependencies(externalDependencies)
    , m_baseStatePreviewImage(QSize(100, 100), QImage::Format_ARGB32)
    , m_restartProcessTimerId(0)
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
    , m_qsbEnabled(qsbEnabled)
{
    m_baseStatePreviewImage.fill(0xFFFFFF);

    // Interval > 0 is used for QFileSystemWatcher related timers to allow all notifications
    // related to a single event to be received before we act.
    m_resetTimer.setSingleShot(true);
    m_resetTimer.setInterval(100);
    QObject::connect(&m_resetTimer, &QTimer::timeout, this, [this] {
        if (isAttached())
            resetPuppet();
    });
    m_updateWatcherTimer.setSingleShot(true);
    m_updateWatcherTimer.setInterval(100);
    QObject::connect(&m_updateWatcherTimer, &QTimer::timeout, this, [this] {
        for (const auto &path : std::as_const(m_pendingUpdateDirs))
            updateWatcher(path);
        m_pendingUpdateDirs.clear();
    });

    // Since generating qsb files is asynchronous and can trigger directory changes, which in turn
    // can trigger qsb generation, compressing qsb generation is necessary to avoid a lot of
    // unnecessary generation when project with multiple shaders is opened.
    m_generateQsbFilesTimer.setSingleShot(true);
    m_generateQsbFilesTimer.setInterval(100);
    QObject::connect(&m_generateQsbFilesTimer, &QTimer::timeout, this, [this] {
        handleShaderChanges();
    });

    connect(m_fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this,
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
    connect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (m_qsbTargets.contains(path)) {
            m_qsbTargets.insert(path, true);
            m_generateQsbFilesTimer.start();
        } else if (m_remainingQsbTargets <= 0) {
            m_resetTimer.start();
        }
    });

    m_rotBlockTimer.setSingleShot(true);
    m_rotBlockTimer.setInterval(0);
    QObject::connect(&m_rotBlockTimer, &QTimer::timeout, this, &NodeInstanceView::updateRotationBlocks);
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

static bool isSkippedRootNode(const ModelNode &node)
{
    static const PropertyNameList skipList({"Qt.ListModel", "QtQuick.ListModel", "Qt.ListModel", "QtQuick.ListModel"});

    if (skipList.contains(node.type()))
        return true;

    return false;
}

static bool isSkippedNode(const ModelNode &node)
{
    static const PropertyNameList skipList({"QtQuick.XmlRole", "Qt.XmlRole", "QtQuick.ListElement", "Qt.ListElement"});

    if (skipList.contains(node.type()))
        return true;

    return false;
}

static bool parentTakesOverRendering(const ModelNode &modelNode)
{
    ModelNode currentNode = modelNode;

    while ((currentNode = currentNode.parentProperty().parentModelNode())) {
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
    if (stateNode.metaInfo().isQtQuickState()) {
        NodeInstance newStateInstance = instanceForModelNode(stateNode);
        activateState(newStateInstance);
    }

    if (m_qsbEnabled) {
        m_generateQsbFilesTimer.stop();
        m_qsbTargets.clear();
        updateQsbPathToFilterMap();
        updateWatcher({});
    }
}

void NodeInstanceView::modelAboutToBeDetached(Model * model)
{
    m_connectionManager.setCrashCallback({});

    m_nodeInstanceCache.insert(model,
                               NodeInstanceCacheData(m_nodeInstanceHash, m_statePreviewImage));

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

    m_generateQsbFilesTimer.stop();
    m_qsbTargets.clear();
}

void NodeInstanceView::handleCrash()
{
    qint64 elaspsedTimeSinceLastCrash = m_lastCrashTime.restart();
    qint64 forceRestartTime = 5000;
#ifdef QT_DEBUG
    forceRestartTime = 10000;
#endif
    if (elaspsedTimeSinceLastCrash > forceRestartTime)
        restartProcess();
    else
        emitDocumentMessage(
            ::QmlDesigner::NodeInstanceView::tr("Qt Quick emulation layer crashed."));

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


void NodeInstanceView::clearErrors()
{
    for (NodeInstance &instance : instances()) {
        instance.setError({});
    }
}

void NodeInstanceView::restartProcess()
{
    clearErrors();
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
        if (stateNode.isValid() && stateNode.metaInfo().isQtQuickState()) {
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

    for (const AbstractProperty &property : propertyList) {
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

    for (const AbstractProperty &property : propertyList) {
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
        } else {
            maybeResetOnPropertyChange(name, property.parentModelNode(),
                                       AbstractView::EmptyPropertiesRemoved);
        }
    }

    for (const ModelNode &node : std::as_const(nodeList))
        removeInstanceNodeRelationship(node);
}

void NodeInstanceView::removeInstanceAndSubInstances(const ModelNode &node)
{
    const QList<ModelNode> subNodes = node.allSubModelNodes();
    for (const ModelNode &subNode : subNodes) {
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

void NodeInstanceView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList,
                                                PropertyChangeFlags propertyChange)
{
    QTC_ASSERT(m_nodeInstanceServer, return);
    m_nodeInstanceServer->changePropertyBindings(createChangeBindingCommand(propertyList));

    for (const auto &property : propertyList)
        maybeResetOnPropertyChange(property.name(), property.parentModelNode(), propertyChange);
}

/*!
    Notifies the view that abstract property values specified by \a propertyList
    were changed for a model node.

    The property will be set for the node instance.

    \sa AbstractProperty, NodeInstance, ModelNode
*/

void NodeInstanceView::variantPropertiesChanged(const QList<VariantProperty>& propertyList,
                                                PropertyChangeFlags propertyChange)
{
    QTC_ASSERT(m_nodeInstanceServer, return);
    updatePosition(propertyList);
    m_nodeInstanceServer->changePropertyValues(createChangeValueCommand(propertyList));

    for (const auto &property : propertyList)
        maybeResetOnPropertyChange(property.name(), property.parentModelNode(), propertyChange);
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

        // Reset puppet when particle emitter/affector is reparented to work around issue in
        // autodetecting the particle system it belongs to. QTBUG-101157
        if (auto metaInfo = node.metaInfo();
            (metaInfo.isQtQuick3DParticles3DParticleEmitter3D()
             || metaInfo.isQtQuick3DParticles3DAffector3D())
            && node.property("system").toBindingProperty().expression().isEmpty()) {
            resetPuppet();
        }
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

void NodeInstanceView::nodeOrderChanged(const NodeListProperty &listProperty)
{
    QTC_ASSERT(m_nodeInstanceServer, return);
    QVector<ReparentContainer> containerList;
    PropertyName propertyName = listProperty.name();
    qint32 containerInstanceId = -1;
    ModelNode containerNode = listProperty.parentModelNode();
    if (hasInstanceForModelNode(containerNode))
        containerInstanceId = instanceForModelNode(containerNode).instanceId();

    const QList<ModelNode> nodes = listProperty.toModelNodeList();
    for (const ModelNode &node : nodes) {
        qint32 instanceId = -1;
        if (hasInstanceForModelNode(node)) {
            instanceId = instanceForModelNode(node).instanceId();
            ReparentContainer container(instanceId, containerInstanceId, propertyName, containerInstanceId, propertyName);
            containerList.append(container);
        }
    }

    m_nodeInstanceServer->reparentInstances(ReparentInstancesCommand(containerList));
}

void NodeInstanceView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
{
    restartProcess();
}

void NodeInstanceView::auxiliaryDataChanged(const ModelNode &node,
                                            AuxiliaryDataKeyView key,
                                            const QVariant &value)
{
    QTC_ASSERT(m_nodeInstanceServer, return );

    switch (key.type) {
    case AuxiliaryDataType::Document:
        if ((key == lockedProperty || key == invisibleProperty) && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyValueContainer container{instance.instanceId(),
                                             PropertyName{key.name},
                                             value,
                                             TypeName(),
                                             key.type};
            m_nodeInstanceServer->changeAuxiliaryValues({{container}});
        };
        break;

    case AuxiliaryDataType::NodeInstanceAuxiliary:
        if (hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            PropertyValueContainer container{instance.instanceId(),
                                             PropertyName{key.name},
                                             value,
                                             TypeName(),
                                             key.type};
            m_nodeInstanceServer->changeAuxiliaryValues({{container}});
        };
        break;

    case AuxiliaryDataType::NodeInstancePropertyOverwrite:
        if (hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            if (value.isValid()) {
                PropertyValueContainer container{instance.instanceId(),
                                                 PropertyName{key.name},
                                                 value,
                                                 TypeName(),
                                                 key.type};
                m_nodeInstanceServer->changeAuxiliaryValues({{container}});
            } else {
                PropertyName name{key.name};
                if (node.hasVariantProperty(name)) {
                    PropertyValueContainer container(instance.instanceId(),
                                                     name,
                                                     node.variantProperty(name).value(),
                                                     TypeName());
                    ChangeValuesCommand changeValueCommand({container});
                    m_nodeInstanceServer->changePropertyValues(changeValueCommand);
                } else if (node.hasBindingProperty(name)) {
                    PropertyBindingContainer container{instance.instanceId(),
                                                       name,
                                                       node.bindingProperty(name).expression(),
                                                       TypeName()};
                    m_nodeInstanceServer->changePropertyBindings({{container}});
                }
            }
        };
        break;

    case AuxiliaryDataType::Temporary:
        if (node.isRootNode()) {
            if (key == languageProperty) {
                const QString languageAsString = value.toString();
                if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current(
                        m_currentTarget))
                    multiLanguageAspect->setCurrentLocale(languageAsString);
                m_nodeInstanceServer->changeLanguage({languageAsString});
            } else if (key.name == "previewSize") {
                m_nodeInstanceServer->changePreviewImageSize(value.toSize());
            }
        };
        break;

    default:
        break;
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

         // Puppet doesn't deal with node source changes properly, so just reset the puppet for now
         resetPuppet(); // TODO: Remove this once the issue is properly fixed (QDS-4955)
     }
}

void NodeInstanceView::capturedData(const CapturedDataCommand &) {}

void NodeInstanceView::currentStateChanged(const ModelNode &node)
{
    NodeInstance newStateInstance = instanceForModelNode(node);

    if (newStateInstance.isValid() && node.metaInfo().isQtQuickState())
        activateState(newStateInstance);
    else
        activateBaseState();
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

NodeInstance NodeInstanceView::instanceForId(qint32 id) const
{
    if (id < 0 || !hasModelNodeForInternalId(id))
        return NodeInstance();

    return m_nodeInstanceHash.value(modelNodeForInternalId(id));
}

bool NodeInstanceView::hasInstanceForId(qint32 id) const
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
    const QList<ModelNode> childNodeVector = newPropertyParent.directSubNodes();

    qint32 parentInstanceId = newPropertyParent.parentModelNode().internalId();

    for (const ModelNode &childNode : childNodeVector) {
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

    for (const VariantProperty &variantProperty : propertyList) {
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

            if (frames.propertyName() == "x" && frames.target().isValid()) {
                NodeInstance instance = instanceForModelNode(frames.target());
                setXValue(instance, variantProperty, informationChangeHash);
            } else if (frames.propertyName() == "y" && frames.target().isValid()) {
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

    const QList<ModelNode> nodes = removedNode.directSubModelNodes();
    for (const ModelNode &childNode : nodes)
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
    for (const ModelNode &node : nodeList) {
        if (isSkippedNode(node))
            continue;

        filteredNodeList.append(node);
    }

    return filteredNodeList;
}
namespace {
bool shouldSendAuxiliary(const AuxiliaryDataKey &key)
{
    return key.type == AuxiliaryDataType::NodeInstancePropertyOverwrite
           || key.type == AuxiliaryDataType::NodeInstanceAuxiliary || key == invisibleProperty
           || key == lockedProperty;
}
} // namespace

bool parentIsBehavior(ModelNode node)
{
    while (node.isValid() && !node.isRootNode()) {
        if (!node.behaviorPropertyName().isEmpty())
            return true;

        node = node.parentProperty().parentModelNode();
    }

    return false;
}

CreateSceneCommand NodeInstanceView::createCreateSceneCommand()
{
    QList<ModelNode> nodeList = allModelNodes();
    QList<NodeInstance> instanceList;

    std::optional oldNodeInstanceHash = m_nodeInstanceCache.take(model());
    if (oldNodeInstanceHash && oldNodeInstanceHash->instances.value(rootModelNode()).isValid()) {
        instanceList = loadInstancesFromCache(nodeList, oldNodeInstanceHash.value());
    } else {
        for (const ModelNode &node : std::as_const(nodeList)) {
            NodeInstance instance = loadNode(node);
            if (!isSkippedNode(node))
                instanceList.append(instance);
        }
    }

    clearErrors();

    nodeList = filterNodesForSkipItems(nodeList);

    QList<VariantProperty> variantPropertyList;
    QList<BindingProperty> bindingPropertyList;

    QVector<PropertyValueContainer> auxiliaryContainerVector;
    for (const ModelNode &node : std::as_const(nodeList)) {
        variantPropertyList.append(node.variantProperties());
        bindingPropertyList.append(node.bindingProperties());
        if (node.isValid() && hasInstanceForModelNode(node)) {
            NodeInstance instance = instanceForModelNode(node);
            for (const auto &element : node.auxiliaryData()) {
                if (shouldSendAuxiliary(element.first)) {
                    auxiliaryContainerVector.emplace_back(instance.instanceId(),
                                                          element.first.name.toQByteArray(),
                                                          element.second,
                                                          TypeName(),
                                                          element.first.type);
                }
            }
        }
    }

    QVector<InstanceContainer> instanceContainerList;
    for (const NodeInstance &instance : std::as_const(instanceList)) {
        InstanceContainer::NodeSourceType nodeSourceType = static_cast<InstanceContainer::NodeSourceType>(instance.modelNode().nodeSourceType());

        InstanceContainer::NodeMetaType nodeMetaType = InstanceContainer::ObjectMetaType;
        if (instance.modelNode().metaInfo().isQtQuickItem())
            nodeMetaType = InstanceContainer::ItemMetaType;

        InstanceContainer::NodeFlags nodeFlags;

        if (parentTakesOverRendering(instance.modelNode()))
            nodeFlags |= InstanceContainer::ParentTakesOverRendering;

        const auto modelNode = instance.modelNode();
        InstanceContainer container(instance.instanceId(),
                                    modelNode.type(),
                                    modelNode.majorVersion(),
                                    modelNode.minorVersion(),
                                    ModelUtils::componentFilePath(modelNode),
                                    modelNode.nodeSource(),
                                    nodeSourceType,
                                    nodeMetaType,
                                    nodeFlags);

        if (!parentIsBehavior(modelNode)) {
            instanceContainerList.append(container);
        }
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
    if (stateNode.isValid() && stateNode.metaInfo().isQtQuickState())
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
                              m_externalDependencies.currentResourcePath(),
                              m_edit3DToolStates[model()->fileUrl()],
                              lastUsedLanguage,
                              m_captureImageMinimumSize,
                              m_captureImageMaximumSize,
                              stateInstanceId);
}

ClearSceneCommand NodeInstanceView::createClearSceneCommand() const
{
    return {};
}

CompleteComponentCommand NodeInstanceView::createComponentCompleteCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<qint32> containerList;
    for (const NodeInstance &instance : instanceList) {
        if (instance.instanceId() >= 0)
            containerList.append(instance.instanceId());
    }

    return CompleteComponentCommand(containerList);
}

ComponentCompletedCommand NodeInstanceView::createComponentCompletedCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<qint32> containerList;
    for (const NodeInstance &instance : instanceList) {
        if (instance.instanceId() >= 0)
            containerList.append(instance.instanceId());
    }

    return ComponentCompletedCommand(containerList);
}

CreateInstancesCommand NodeInstanceView::createCreateInstancesCommand(const QList<NodeInstance> &instanceList) const
{
    QVector<InstanceContainer> containerList;
    for (const NodeInstance &instance : instanceList) {
        InstanceContainer::NodeSourceType nodeSourceType = static_cast<InstanceContainer::NodeSourceType>(instance.modelNode().nodeSourceType());

        InstanceContainer::NodeMetaType nodeMetaType = InstanceContainer::ObjectMetaType;
        if (instance.modelNode().metaInfo().isQtQuickItem())
            nodeMetaType = InstanceContainer::ItemMetaType;

        InstanceContainer::NodeFlags nodeFlags;

        if (parentTakesOverRendering(instance.modelNode()))
            nodeFlags |= InstanceContainer::ParentTakesOverRendering;

        const auto modelNode = instance.modelNode();
        InstanceContainer container(instance.instanceId(),
                                    modelNode.type(),
                                    modelNode.majorVersion(),
                                    modelNode.minorVersion(),
                                    ModelUtils::componentFilePath(modelNode),
                                    modelNode.nodeSource(),
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
    for (const NodeInstance &instance : instanceList) {
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

    bool reflectionFlag = m_puppetTransaction.isValid()
                          && (!currentTimeline().isValid() || !currentTimeline().isRecording());

    for (const VariantProperty &property : propertyList) {
        ModelNode node = property.parentModelNode();
        if (QmlPropertyChanges::isValidQmlPropertyChanges(node))
            reflectionFlag = false;

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
    for (const ModelNode &node : nodeList) {
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
    for (const ModelNode &node : nodeList) {
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

    for (const AbstractProperty &property : propertyList) {
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

    for (const ModelNode &modelNode : nodeList)
        keyNumberVector.append(modelNode.internalId());

    return RemoveSharedMemoryCommand(sharedMemoryTypeName, keyNumberVector);
}

void NodeInstanceView::valuesChanged(const ValuesChangedCommand &command)
{
    if (!model())
        return;

    QList<QPair<ModelNode, PropertyName> > valuePropertyChangeList;

    const QVector<PropertyValueContainer> containers = command.valueChanges();
    for (const PropertyValueContainer &container : containers) {
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
                if (auto qmlObjectNode = QmlObjectNode(instance.modelNode())) {
                    if (qmlObjectNode.modelValue(container.name()) != container.value())
                        qmlObjectNode.setVariantProperty(container.name(), container.value());
                }
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

    QVector<InformationContainer> containerVector;

    const QVector<ImageContainer> containers = command.images();
    for (const ImageContainer &container : containers) {
        if (hasInstanceForId(container.instanceId())) {
            NodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                if (container.rect().isValid()) {
                    InformationContainer rectContainer = InformationContainer(container.instanceId(),
                                                                              BoundingRectPixmap,
                                                                              container.rect(),
                                                                              {},
                                                                              {});
                    containerVector.append(rectContainer);
                }
                instance.setRenderPixmap(container.image());
                renderImageChangeSet.insert(instance.modelNode());
            }
        }
    }

    m_nodeInstanceServer->benchmark(Q_FUNC_INFO + QString::number(renderImageChangeSet.size()));

    if (!renderImageChangeSet.isEmpty())
        emitInstancesRenderImageChanged(Utils::toList(renderImageChangeSet));

    if (!containerVector.isEmpty()) {
        QMultiHash<ModelNode, InformationName> informationChangeHash = informationChanged(
            containerVector);

        if (!informationChangeHash.isEmpty())
            emitInstanceInformationsChange(informationChangeHash);
    }
}

QMultiHash<ModelNode, InformationName> NodeInstanceView::informationChanged(const QVector<InformationContainer> &containerVector)
{
    QMultiHash<ModelNode, InformationName> informationChangeHash;

    for (const InformationContainer &container : containerVector) {
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

    m_nodeInstanceServer->benchmark(Q_FUNC_INFO + QString::number(informationChangeHash.size()));

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
        if (m_currentTarget && m_currentTarget->kit()) {
            if (QtSupport::QtVersion *qtVer = QtSupport::QtKitAspect::qtVersion(m_currentTarget->kit())) {
                m_qsbPath = qtVer->binPath().pathAppended("qsb").withExecutableSuffix();
                if (!m_qsbPath.exists())
                    m_qsbPath.clear();
            }
        }

        restartProcess();
    }
}

ProjectExplorer::Target *NodeInstanceView::target() const
{
    return m_currentTarget;
}

void NodeInstanceView::statePreviewImagesChanged(const StatePreviewImageChangedCommand &command)
{
    if (!model())
      return;

  QVector<ModelNode> previewImageChangeVector;

  const QVector<ImageContainer> containers = command.previews();
  for (const ImageContainer &container : containers) {
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

    const QVector<qint32> instances = command.instances();
    for (const qint32 &instanceId : instances) {
        if (hasModelNodeForInternalId(instanceId))
            nodeVector.append(modelNodeForInternalId(instanceId));
    }

    m_nodeInstanceServer->benchmark(Q_FUNC_INFO + QString::number(nodeVector.size()));

    if (!nodeVector.isEmpty())
        emitInstancesCompleted(nodeVector);
}

void NodeInstanceView::childrenChanged(const ChildrenChangedCommand &command)
{
     if (!model())
        return;

    QVector<ModelNode> childNodeVector;

    const QVector<qint32> instances = command.childrenInstances();
    for (const qint32 &instanceId : instances) {
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

    const QVector<qint32> instances = command.instances();
    for (const qint32 &instanceId : instances) {
        if (hasModelNodeForInternalId(instanceId))
            nodeVector.append(modelNodeForInternalId(instanceId));
    }

    emitInstanceToken(command.tokenName(), command.tokenNumber(), nodeVector);
}

void NodeInstanceView::debugOutput(const DebugOutputCommand & command)
{
    DocumentMessage error(::QmlDesigner::NodeInstanceView::tr("Qt Quick emulation layer crashed."));
    if (command.instanceIds().isEmpty()) {
        emitDocumentMessage(command.text());
    } else {
        QVector<qint32> instanceIdsWithChangedErrors;
        const QVector<qint32> instanceIds = command.instanceIds();
        for (const qint32 &instanceId : instanceIds) {
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
    for (const ModelNode &node : nodeVector)
        instanceIdVector.append(node.internalId());

    m_nodeInstanceServer->token(TokenCommand(token, number, instanceIdVector));
}

void NodeInstanceView::selectionChanged(const ChangeSelectionCommand &command)
{
    clearSelectedModelNodes();
    const QVector<qint32> instanceIds = command.instanceIds();
    for (const qint32 &instanceId : instanceIds) {
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
                const double ratio = m_externalDependencies.formEditorDevicePixelRatio();
                const int dim = Constants::MODELNODE_PREVIEW_IMAGE_DIMENSIONS * ratio;
                if (image.height() != dim || image.width() != dim)
                    image = image.scaled(dim, dim, Qt::KeepAspectRatio);
                image.setDevicePixelRatio(ratio);
                updatePreviewImageForNode(node, image);
            }
        }
    } else if (command.type() == PuppetToCreatorCommand::Import3DSupport) {
        QVariantMap supportMap;
        if (externalDependencies().isQt6Project())
            supportMap = qvariant_cast<QVariantMap>(command.data());
        emitImport3DSupportChanged(supportMap);
    } else if (command.type() == PuppetToCreatorCommand::NodeAtPos) {
        auto data = qvariant_cast<QVariantList>(command.data());
        if (data.size() == 2) {
            ModelNode modelNode = modelNodeForInternalId(data[0].toInt());
            QVector3D pos3d = data[1].value<QVector3D>();
            emitNodeAtPosResult(modelNode, pos3d);
        }
    }
}

std::unique_ptr<NodeInstanceServerProxy> NodeInstanceView::createNodeInstanceServerProxy()
{
    return std::make_unique<NodeInstanceServerProxy>(this,
                                                     m_currentTarget,
                                                     m_connectionManager,
                                                     m_externalDependencies);
}

void NodeInstanceView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                            const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    m_nodeInstanceServer->changeSelection(createChangeSelectionCommand(selectedNodeList));
    m_rotBlockTimer.start();
}

void NodeInstanceView::sendInputEvent(QInputEvent *e) const
{
    m_nodeInstanceServer->inputEvent(InputEventCommand(e));
}

void NodeInstanceView::view3DAction(View3DActionType type, const QVariant &value)
{
    m_nodeInstanceServer->view3DAction({type, value});
}

void NodeInstanceView::requestModelNodePreviewImage(const ModelNode &node,
                                                    const ModelNode &renderNode) const
{
    if (m_nodeInstanceServer && node.isValid()) {
        auto instance = instanceForModelNode(node);
        if (instance.isValid()) {
            qint32 renderItemId = -1;
            QString componentPath;
            if (renderNode.isValid()) {
                auto renderInstance = instanceForModelNode(renderNode);
                if (renderInstance.isValid())
                    renderItemId = renderInstance.instanceId();
                if (renderNode.isComponent())
                    componentPath = ModelUtils::componentFilePath(renderNode);
            } else if (node.isComponent()) {
                componentPath = ModelUtils::componentFilePath(node);
            }
            const double ratio = m_externalDependencies.formEditorDevicePixelRatio();
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

QVariant NodeInstanceView::modelNodePreviewImageDataToVariant(const ModelNodePreviewImageData &imageData) const
{
    static QPixmap placeHolder;
    if (placeHolder.isNull()) {
        QPixmap placeHolderSrc(":/navigator/icon/tooltip_placeholder.png");
        placeHolder = {150, 150};
        // Placeholder has transparency, but we don't want to show the checkerboard, so
        // paint in the correct background color
        placeHolder.fill(Utils::creatorTheme()->color(Utils::Theme::BackgroundColorNormal));
        QPainter painter(&placeHolder);
        painter.drawPixmap(0, 0, 150, 150, placeHolderSrc);
    }

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

QVariant NodeInstanceView::previewImageDataForImageNode(const ModelNode &modelNode) const
{
    if (!modelNode.isValid())
        return {};

    VariantProperty prop = modelNode.variantProperty("source");
    QString imageSource = prop.value().toString();

    ModelNodePreviewImageData imageData;
    imageData.id = modelNode.id();
    imageData.type = QString::fromLatin1(modelNode.type());
    const double ratio = m_externalDependencies.formEditorDevicePixelRatio();

    if (imageSource.isEmpty() && modelNode.metaInfo().isQtQuick3DTexture()) {
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
                    imageData.pixmap = itemNode.instanceRenderPixmap().scaled(dim,
                                                                              dim,
                                                                              Qt::KeepAspectRatio);
                    imageData.pixmap.setDevicePixelRatio(ratio);
                }
                imageData.info = ::QmlDesigner::NodeInstanceView::tr("Source item: %1")
                                     .arg(boundNode.id());
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
            imageSource = QFileInfo(modelNode.model()->fileUrl().toLocalFile())
                              .dir()
                              .absoluteFilePath(imageSource);

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
            if (modelNode.metaInfo().isQtSafeRendererSafeRendererPicture()) {
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
                if (imageFi.suffix() == "hdr")
                    originalPixmap = HdrImage{imageSource}.toPixmap();
                else
                    originalPixmap.load(imageSource);
            }
            if (!originalPixmap.isNull()) {
                const int dim = Constants::MODELNODE_PREVIEW_IMAGE_DIMENSIONS * ratio;
                imageData.pixmap = originalPixmap.scaled(dim, dim, Qt::KeepAspectRatio);
                imageData.pixmap.setDevicePixelRatio(ratio);
                imageData.time = modified;
                imageData.info = ImageUtils::imageInfo(imageSource);
                m_imageDataMap.insert(imageData.id, imageData);
            }
        }
    }

    return modelNodePreviewImageDataToVariant(imageData);
}

void NodeInstanceView::startNanotrace()
{
    NANOTRACE_INIT("QmlDesigner", "MainThread", "nanotrace_qmldesigner.json");
    m_connectionManager.writeCommand(QVariant::fromValue(StartNanotraceCommand(QDir::currentPath())));
}

void NodeInstanceView::endNanotrace()
{
    NANOTRACE_SHUTDOWN();
    m_connectionManager.writeCommand(QVariant::fromValue(EndNanotraceCommand()) );
}

QVariant NodeInstanceView::previewImageDataForGenericNode(const ModelNode &modelNode,
                                                          const ModelNode &renderNode) const
{
    if (!modelNode.isValid())
        return {};

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
    QStringList qsbFiles;

    const QString projPath = m_externalDependencies.currentProjectDirPath();

    if (projPath.isEmpty())
        return;

    const QStringList files = m_fileSystemWatcher->files();
    const QStringList directories = m_fileSystemWatcher->directories();
    if (path.isEmpty()) {
        // Do full update
        rootPath = projPath;
        if (!directories.isEmpty())
            m_fileSystemWatcher->removePaths(directories);
        if (!files.isEmpty())
            m_fileSystemWatcher->removePaths(files);
    } else {
        rootPath = path;
        for (const auto &file : files) {
            if (file.startsWith(path))
                oldFiles.append(file);
        }
        for (const auto &directory : directories) {
            if (directory.startsWith(path))
                oldDirs.append(directory);
        }
    }

    newDirs.append(rootPath);

    QDirIterator dirIterator(rootPath, {}, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIterator.hasNext())
        newDirs.append(dirIterator.next());

    // Common shader suffixes
    static const QStringList filterList {"*.frag", "*.vert",
                                         "*.glsl", "*.glslv", "*.glslf",
                                         "*.vsh", "*.fsh"};

    QDirIterator fileIterator(rootPath, filterList, QDir::Files, QDirIterator::Subdirectories);
    while (fileIterator.hasNext())
        newFiles.append(fileIterator.next());

    // Find out which shader files need qsb files generated for them.
    // Go through all configured paths and find files that match the specified filter in that path.
    bool generateQsb = false;
    QHash<QString, QStringList>::const_iterator it = m_qsbPathToFilterMap.constBegin();
    while (it != m_qsbPathToFilterMap.constEnd()) {
        if (!it.key().isEmpty() && !it.key().startsWith(rootPath)) {
            ++it;
            continue;
        }

        QDirIterator qsbIterator(it.key().isEmpty() ? rootPath : it.key(),
                                 it.value(), QDir::Files,
                                 it.key().isEmpty() ? QDirIterator::Subdirectories
                                                    : QDirIterator::NoIteratorFlags);

        while (qsbIterator.hasNext()) {
            QString qsbFile = qsbIterator.next();

            if (qsbFile.endsWith(".qsb"))
                continue; // Skip any generated files that are caught by wildcards

            // Filters may specify shader files with non-default suffixes, so add them to newFiles
            if (!newFiles.contains(qsbFile))
                newFiles.append(qsbFile);

            // Only generate qsb files for newly detected files. This avoids immediately regenerating
            // qsb file if it's manually deleted, as directory change triggers calling this method.
            if (!oldFiles.contains(qsbFile)) {
                m_qsbTargets.insert(qsbFile, true);
                generateQsb = true;
            }
        }
        ++it;
    }

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

    if (generateQsb)
        m_generateQsbFilesTimer.start();
}

void NodeInstanceView::handleQsbProcessExit(Utils::Process *qsbProcess, const QString &shader)
{
    --m_remainingQsbTargets;

    const QString errStr = qsbProcess->errorString();
    const QByteArray stdErrStr = qsbProcess->readAllRawStandardError();

    if (!errStr.isEmpty() || !stdErrStr.isEmpty()) {
        Core::MessageManager::writeSilently(QCoreApplication::translate(
            "QmlDesigner::NodeInstanceView", "Failed to generate QSB file for: %1").arg(shader));
        if (!errStr.isEmpty())
            Core::MessageManager::writeSilently(errStr);
        if (!stdErrStr.isEmpty())
            Core::MessageManager::writeSilently(QString::fromUtf8(stdErrStr));
    }

    if (m_remainingQsbTargets <= 0)
        m_resetTimer.start();

    qsbProcess->deleteLater();
}

void NodeInstanceView::updateQsbPathToFilterMap()
{
    m_qsbPathToFilterMap.clear();
    if (m_currentTarget && !m_qsbPath.isEmpty()) {
        const auto bs = qobject_cast<QmlProjectManager::QmlBuildSystem *>(m_currentTarget->buildSystem());
        if (!bs)
            return;
        const QStringList shaderToolFiles = bs->shaderToolFiles();

        const QString projPath = m_externalDependencies.currentProjectDirPath();

        if (projPath.isEmpty())
            return;

        // Parse ShaderTool files from project configuration.
        // Separate files to path and file name (called filter here as it can contain wildcards)
        // and group filters by paths. Blank path indicates project-wide file wildcard.
        for (const auto &file : shaderToolFiles) {
            int idx = file.lastIndexOf('/');
            QString key;
            QString filter;
            if (idx >= 0) {
                key = projPath + "/" + file.left(idx);
                filter = file.mid(idx + 1);
            } else {
                filter = file;
            }
            m_qsbPathToFilterMap[key].append(filter);
        }
    }
}

void NodeInstanceView::handleShaderChanges()
{
    if (!m_currentTarget)
        return;

    const auto bs = qobject_cast<QmlProjectManager::QmlBuildSystem *>(m_currentTarget->buildSystem());
    if (!bs)
        return;

    QStringList baseArgs = bs->shaderToolArgs();
    if (baseArgs.isEmpty())
        return;

    QStringList newShaders;
    QHash<QString, bool>::iterator it = m_qsbTargets.begin();
    while (it != m_qsbTargets.end()) {
        if (it.value()) {
            newShaders.append(it.key());
            it.value() = false;
        }
        ++it;
    }

    if (newShaders.isEmpty())
        return;

    m_remainingQsbTargets += newShaders.size();

    for (const auto &shader : std::as_const(newShaders)) {
        const Utils::FilePath srcFile = Utils::FilePath::fromString(shader);
        const Utils::FilePath srcPath = srcFile.absolutePath();
        const Utils::FilePath outPath = Utils::FilePath::fromString(shader + ".qsb");

        if (!srcFile.exists()) {
            m_qsbTargets.remove(shader);
            --m_remainingQsbTargets;
            continue;
        }

        if ((outPath.exists() && outPath.lastModified() > srcFile.lastModified())) {
            --m_remainingQsbTargets;
            continue;
        }

        QStringList args = baseArgs;
        args.append("-o");
        args.append(outPath.toString());
        args.append(shader);
        auto qsbProcess = new Utils::Process(this);
        connect(qsbProcess, &Utils::Process::done, this, [this, qsbProcess, shader] {
            handleQsbProcessExit(qsbProcess, shader);
        });
        qsbProcess->setWorkingDirectory(srcPath);
        qsbProcess->setCommand({m_qsbPath, args});
        qsbProcess->start();
    }
}

void NodeInstanceView::updateRotationBlocks()
{
    if (!model())
        return;

    QList<ModelNode> qml3DNodes;
    QSet<ModelNode> rotationKeyframeTargets;
    bool groupsResolved = false;
    const PropertyName targetPropName {"target"};
    const PropertyName propertyPropName {"property"};
    const PropertyName rotationPropName {"rotation"};
    const QList<ModelNode> selectedNodes = selectedModelNodes();
    for (const auto &node : selectedNodes) {
        if (Qml3DNode::isValidQml3DNode(node)) {
            if (!groupsResolved) {
                const QList<ModelNode> keyframeGroups = allModelNodesOfType(
                    model()->qtQuickTimelineKeyframeGroupMetaInfo());
                for (const auto &kfgNode : keyframeGroups) {
                    if (kfgNode.isValid()) {
                        VariantProperty varProp = kfgNode.variantProperty(propertyPropName);
                        if (varProp.isValid() && varProp.value().value<PropertyName>() == rotationPropName) {
                            BindingProperty bindProp = kfgNode.bindingProperty(targetPropName);
                            if (bindProp.isValid()) {
                                ModelNode targetNode = bindProp.resolveToModelNode();
                                if (Qml3DNode::isValidQml3DNode(targetNode))
                                    rotationKeyframeTargets.insert(targetNode);
                            }
                        }
                    }
                }
                groupsResolved = true;
            }
            qml3DNodes.append(node);
        }
    }
    if (!qml3DNodes.isEmpty()) {
        for (const auto &node : std::as_const(qml3DNodes)) {
            if (rotationKeyframeTargets.contains(node))
                node.setAuxiliaryData(rotBlockProperty, true);
            else
                node.setAuxiliaryData(rotBlockProperty, false);
        }
    }
}

void NodeInstanceView::maybeResetOnPropertyChange(const PropertyName &name, const ModelNode &node,
                                                  PropertyChangeFlags flags)
{
    bool reset = false;
    if (flags & AbstractView::PropertiesAdded && name == "model"
        && node.metaInfo().isQtQuickRepeater()) {
        // TODO: This is a workaround for QTBUG-97583:
        //       Reset puppet when repeater model is first added, if there is already a delegate
        if (node.hasProperty("delegate"))
            reset = true;
    } else if (name == "shader" && node.metaInfo().isQtQuick3DShader()) {
        reset = true;
    }
    if (reset)
        resetPuppet();
}

QList<NodeInstance> NodeInstanceView::loadInstancesFromCache(const QList<ModelNode> &nodeList,
                                                             const NodeInstanceCacheData &cache)
{
    QList<NodeInstance> instanceList;

    auto previews = cache.previewImages;
    auto iterator = previews.begin();
    while (iterator != previews.end()) {
        if (iterator.key().isValid())
            m_statePreviewImage.insert(iterator.key(), iterator.value());
        iterator++;
    }

    for (const ModelNode &node : std::as_const(nodeList)) {
        NodeInstance instance = cache.instances.value(node);
        if (instance.isValid())
            insertInstanceRelationships(instance);
        else
            instance = loadNode(node);

        if (node.isRootNode())
            m_rootNodeInstance = instance;
        if (!isSkippedNode(node))
            instanceList.append(instanceForModelNode(node));
    }

    return instanceList;
}
} // namespace QmlDesigner
