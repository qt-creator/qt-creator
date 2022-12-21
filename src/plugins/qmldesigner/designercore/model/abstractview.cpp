// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractview.h"

#include "auxiliarydataproperties.h"
#include "model.h"
#include "model_p.h"
#include "internalnode_p.h"
#include "nodeinstanceview.h"
#include <qmlstate.h>
#include <qmltimeline.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <bindingproperty.h>

#include <coreplugin/helpmanager.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QWidget>
#include <QtGui/qimage.h>

namespace QmlDesigner {

/*!
\class QmlDesigner::AbstractView
\ingroup CoreModel
\brief The AbstractView class provides an abstract interface that views and
editors can implement to be notified about model changes.

\sa QmlDesigner::WidgetQueryView(), QmlDesigner::NodeInstanceView()
*/

AbstractView::~AbstractView()
{
    if (m_model)
        m_model.data()->detachView(this, Model::DoNotNotifyView);
}

/*!
    Sets the view of a new \a model. This is handled automatically by
    AbstractView::modelAttached().

    \sa AbstractView::modelAttached()
*/
void AbstractView::setModel(Model *model)
{
    Q_ASSERT(model != nullptr);
    if (model == m_model.data())
        return;

    if (m_model)
        m_model.data()->detachView(this);

    m_model = model;
}

RewriterTransaction AbstractView::beginRewriterTransaction(const QByteArray &identifier)
{
    return RewriterTransaction(this, identifier);
}

ModelNode AbstractView::createModelNode(const TypeName &typeName)
{
    const NodeMetaInfo metaInfo = model()->metaInfo(typeName);
    return createModelNode(typeName, metaInfo.majorVersion(), metaInfo.minorVersion());
}

ModelNode AbstractView::createModelNode(const TypeName &typeName,
                                        int majorVersion,
                                        int minorVersion,
                                        const QList<QPair<PropertyName, QVariant>> &propertyList,
                                        const AuxiliaryDatas &auxPropertyList,
                                        const QString &nodeSource,
                                        ModelNode::NodeSourceType nodeSourceType,
                                        const QString &behaviorPropertyName)
{
    return ModelNode(model()->d->createNode(typeName, majorVersion, minorVersion, propertyList, auxPropertyList, nodeSource, nodeSourceType, behaviorPropertyName), model(), this);
}


/*!
    Returns the constant root model node.
*/

ModelNode AbstractView::rootModelNode() const
{
    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), const_cast<AbstractView*>(this));
}


/*!
    Returns the root model node.
*/

ModelNode AbstractView::rootModelNode()
{
    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), this);
}

/*!
    Sets the reference to a model to a null pointer.

*/
void AbstractView::removeModel()
{
    m_model.clear();
}

WidgetInfo AbstractView::createWidgetInfo(QWidget *widget,
                                          const QString &uniqueId,
                                          WidgetInfo::PlacementHint placementHint,
                                          int placementPriority,
                                          const QString &tabName,
                                          DesignerWidgetFlags widgetFlags)
{
    WidgetInfo widgetInfo;

    widgetInfo.widget = widget;
    widgetInfo.uniqueId = uniqueId;
    widgetInfo.placementHint = placementHint;
    widgetInfo.placementPriority = placementPriority;
    widgetInfo.tabName = tabName;
    widgetInfo.widgetFlags = widgetFlags;

    return widgetInfo;
}

/*!
    Returns the model of the view.
*/
Model* AbstractView::model() const
{
    return m_model.data();
}

bool AbstractView::isAttached() const
{
    return model();
}

/*!
Called if a view is being attached to \a model.
The default implementation is setting the reference of the model to the view.
\sa Model::attachView()
*/
void AbstractView::modelAttached(Model *model)
{
    setModel(model);

    if (model)
        model->d->updateEnabledViews();
}

/*!
Called before a view is detached from \a model.

This function is not called if Model::detachViewWithOutNotification is used.
The default implementation
is removing the reference to the model from the view.

\sa Model::detachView()
*/
void AbstractView::modelAboutToBeDetached(Model *)
{
    removeModel();
}

/*!
    \enum QmlDesigner::AbstractView::PropertyChangeFlag

    Notifies about changes in the abstract properties of a node:

    \value  NoAdditionalChanges
            No changes were made.

    \value  PropertiesAdded
            Some properties were added.

    \value  EmptyPropertiesRemoved
            Empty properties were removed.
*/

void AbstractView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &/*propertyList*/)
{
}

void AbstractView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{
}

void AbstractView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void AbstractView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void AbstractView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void AbstractView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{
}

void AbstractView::nodeSourceChanged(const ModelNode &/*modelNode*/, const QString &/*newNodeSource*/)
{
}

void AbstractView::rewriterBeginTransaction()
{
}

void AbstractView::rewriterEndTransaction()
{
}

void AbstractView::instanceErrorChanged(const QVector<ModelNode> &/*errorNodeList*/)
{
}

void AbstractView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

// Node related functions

/*!
\fn void AbstractView::nodeCreated(const ModelNode &createdNode)
Called when the new node \a createdNode is created.
*/
void AbstractView::nodeCreated(const ModelNode &/*createdNode*/)
{
}

void AbstractView::currentStateChanged(const ModelNode &/*node*/)
{
}

/*!
Called when the file URL (that is needed to resolve relative paths against,
for example) is changed form \a oldUrl to \a newUrl.
*/
void AbstractView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/)
{
}

void AbstractView::nodeOrderChanged(const NodeListProperty & /*listProperty*/) {}

void AbstractView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &, int)
{
    nodeOrderChanged(listProperty);
}

/*!
\fn void AbstractView::nodeAboutToBeRemoved(const ModelNode &removedNode)
Called when the node specified by \a removedNode will be removed.
*/
void AbstractView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
}

void AbstractView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

/*!
Called when the properties specified by \a propertyList are removed.
*/
void AbstractView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

/*!
\fn void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
Called when the parent of \a node will be changed from \a oldPropertyParent to
\a newPropertyParent.
*/

/*!
\fn void QmlDesigner::AbstractView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                                         const QList<ModelNode> &lastSelectedNodeList)
Called when the selection is changed from \a lastSelectedNodeList to
\a selectedNodeList.
*/
void AbstractView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{
}

void AbstractView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::nodeIdChanged(const ModelNode& /*node*/, const QString& /*newId*/, const QString& /*oldId*/)
{
}

void AbstractView::variantPropertiesChanged(const QList<VariantProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::bindingPropertiesAboutToBeChanged(const QList<BindingProperty> &) {}

void AbstractView::bindingPropertiesChanged(const QList<BindingProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::signalDeclarationPropertiesChanged(const QVector<SignalDeclarationProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
}

void AbstractView::nodeTypeChanged(const ModelNode & /*node*/, const TypeName & /*type*/, int /*majorVersion*/, int /*minorVersion*/)
{

}

void AbstractView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
}

void AbstractView::possibleImportsChanged(const QList<Import> &/*possibleImports*/)
{
}

void AbstractView::usedImportsChanged(const QList<Import> &/*usedImports*/)
{
}

void AbstractView::auxiliaryDataChanged(const ModelNode & /*node*/,
                                        AuxiliaryDataKeyView /*key*/,
                                        const QVariant & /*data*/)
{
}

void AbstractView::customNotification(const AbstractView * /*view*/, const QString & /*identifier*/, const QList<ModelNode> & /*nodeList*/, const QList<QVariant> & /*data*/)
{
}

void AbstractView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{
}

void AbstractView::documentMessagesChanged(const QList<DocumentMessage> &/*errors*/, const QList<DocumentMessage> &/*warnings*/)
{
}

void AbstractView::currentTimelineChanged(const ModelNode & /*node*/)
{
}

void AbstractView::renderImage3DChanged(const QImage & /*image*/)
{
}

void AbstractView::updateActiveScene3D(const QVariantMap & /*sceneState*/)
{
}

void AbstractView::updateImport3DSupport(const QVariantMap & /*supportMap*/)
{
}

// a Quick3DNode that is picked at the requested view position in the 3D Editor and the 3D scene
// position of the requested view position.
void AbstractView::nodeAtPosReady(const ModelNode & /*modelNode*/, const QVector3D &/*pos3d*/) {}

void AbstractView::modelNodePreviewPixmapChanged(const ModelNode & /*node*/, const QPixmap & /*pixmap*/)
{
}

void AbstractView::view3DAction(View3DActionType, const QVariant &) {}

void AbstractView::active3DSceneChanged(qint32 /*sceneId*/) {}

void AbstractView::dragStarted(QMimeData * /*mimeData*/) {}
void AbstractView::dragEnded() {}

QList<ModelNode> AbstractView::toModelNodeList(const QList<Internal::InternalNode::Pointer> &nodeList) const
{
    return QmlDesigner::toModelNodeList(nodeList, const_cast<AbstractView*>(this));
}

QList<ModelNode> toModelNodeList(const QList<Internal::InternalNode::Pointer> &nodeList, AbstractView *view)
{
    QList<ModelNode> newNodeList;
    for (const Internal::InternalNode::Pointer &node : nodeList)
        newNodeList.append(ModelNode(node, view->model(), view));

    return newNodeList;
}

QList<Internal::InternalNode::Pointer> toInternalNodeList(const QList<ModelNode> &nodeList)
{
    QList<Internal::InternalNode::Pointer> newNodeList;
    for (const ModelNode &node : nodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
}

/*!
    Sets the list of nodes to the actual selected nodes specified by
    \a selectedNodeList if the node or its ancestors are not locked.
*/
void AbstractView::setSelectedModelNodes(const QList<ModelNode> &selectedNodeList)
{
    QList<ModelNode> unlockedNodes;

    for (const auto &modelNode : selectedNodeList) {
        if (!ModelNode::isThisOrAncestorLocked(modelNode))
            unlockedNodes.push_back(modelNode);
    }

    model()->d->setSelectedNodes(toInternalNodeList(unlockedNodes));
}

void AbstractView::setSelectedModelNode(const ModelNode &modelNode)
{
    if (ModelNode::isThisOrAncestorLocked(modelNode)) {
        clearSelectedModelNodes();
        return;
    }
    setSelectedModelNodes({modelNode});
}

/*!
    Clears the selection.
*/
void AbstractView::clearSelectedModelNodes()
{
    model()->d->clearSelectedNodes();
}

bool AbstractView::hasSelectedModelNodes() const
{
    return !model()->d->selectedNodes().isEmpty();
}

bool AbstractView::hasSingleSelectedModelNode() const
{
    return model()->d->selectedNodes().count() == 1;
}

bool AbstractView::isSelectedModelNode(const ModelNode &modelNode) const
{
    return model()->d->selectedNodes().contains(modelNode.internalNode());
}

/*!
    Sets the list of nodes to the actual selected nodes. Returns a list of the
    selected nodes.
*/
QList<ModelNode> AbstractView::selectedModelNodes() const
{
    return toModelNodeList(model()->d->selectedNodes());
}

ModelNode AbstractView::firstSelectedModelNode() const
{
    if (hasSelectedModelNodes())
        return ModelNode(model()->d->selectedNodes().constFirst(), model(), this);

    return ModelNode();
}

ModelNode AbstractView::singleSelectedModelNode() const
{
    if (hasSingleSelectedModelNode())
        return ModelNode(model()->d->selectedNodes().constFirst(), model(), this);

    return ModelNode();
}

/*!
    Adds \a node to the selection list.
*/
void AbstractView::selectModelNode(const ModelNode &modelNode)
{
    QTC_ASSERT(modelNode.isInHierarchy(), return);
    model()->d->selectNode(modelNode.internalNode());
}

/*!
    Removes \a node from the selection list.
*/
void AbstractView::deselectModelNode(const ModelNode &node)
{
    model()->d->deselectNode(node.internalNode());
}

ModelNode AbstractView::modelNodeForId(const QString &id)
{
    return ModelNode(model()->d->nodeForId(id), model(), this);
}

bool AbstractView::hasId(const QString &id) const
{
    return model()->hasId(id);
}

ModelNode AbstractView::modelNodeForInternalId(qint32 internalId) const
{
     return ModelNode(model()->d->nodeForInternalId(internalId), model(), this);
}

bool AbstractView::hasModelNodeForInternalId(qint32 internalId) const
{
    return model()->d->hasNodeForInternalId(internalId);
}

const NodeInstanceView *AbstractView::nodeInstanceView() const
{
    if (model())
        return model()->d->nodeInstanceView();
    else
        return nullptr;
}

RewriterView *AbstractView::rewriterView() const
{
    if (model())
        return model()->d->rewriterView();
    else
        return nullptr;
}

void AbstractView::resetView()
{
    if (!model())
        return;
    Model *currentModel = model();

    currentModel->detachView(this);
    currentModel->attachView(this);
}

void AbstractView::resetPuppet()
{
    QTC_ASSERT(isAttached(), return);
    emitCustomNotification(QStringLiteral("reset QmlPuppet"));
}

bool AbstractView::hasWidget() const
{
    return false;
}

WidgetInfo AbstractView::widgetInfo()
{
    return createWidgetInfo();
}

void AbstractView::disableWidget()
{
    if (hasWidget() && widgetInfo().widgetFlags == DesignerWidgetFlags::DisableOnError)
        widgetInfo().widget->setEnabled(false);
}

void AbstractView::enableWidget()
{
    if (hasWidget() && widgetInfo().widgetFlags == DesignerWidgetFlags::DisableOnError)
        widgetInfo().widget->setEnabled(true);
}

QString AbstractView::contextHelpId() const
{
    QString id = const_cast<AbstractView *>(this)->widgetInfo().uniqueId;

    if (!selectedModelNodes().isEmpty()) {
        const auto nodeId = selectedModelNodes().first().simplifiedTypeName();
        id += " " + nodeId;
    }

    return id;
}

void AbstractView::setCurrentTimeline(const ModelNode &timeline)
{
    if (currentTimeline().isValid())
        currentTimeline().toogleRecording(false);

    if (model())
        model()->d->notifyCurrentTimelineChanged(timeline);
}

void AbstractView::activateTimelineRecording(const ModelNode &timeline)
{
    if (currentTimeline().isValid())
        currentTimeline().toogleRecording(true);

    Internal::WriteLocker locker(m_model.data());

    if (model())
        model()->d->notifyCurrentTimelineChanged(timeline);
}

void AbstractView::deactivateTimelineRecording()
{
    if (currentTimeline().isValid()) {
        currentTimeline().toogleRecording(false);
        currentTimeline().resetGroupRecording();
    }

    if (model())
        model()->d->notifyCurrentTimelineChanged(ModelNode());
}

bool AbstractView::executeInTransaction(const QByteArray &identifier, const AbstractView::OperationBlock &lambda)
{
    try {
        RewriterTransaction transaction = beginRewriterTransaction(identifier);
        lambda();
        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
        return false;
    }

    return true;
}

bool AbstractView::isEnabled() const
{
    return m_enabled;
}

void AbstractView::setEnabled(bool b)
{
    m_enabled = b;

    if (model())
        model()->d->updateEnabledViews();
}

QList<ModelNode> AbstractView::allModelNodes() const
{
    QTC_ASSERT(model(), return {});
    return toModelNodeList(model()->d->allNodes());
}

QList<ModelNode> AbstractView::allModelNodesOfType(const NodeMetaInfo &type) const
{
    return Utils::filtered(allModelNodes(),
                           [&](const ModelNode &node) { return node.metaInfo().isBasedOn(type); });
}

void AbstractView::emitDocumentMessage(const QString &error)
{
    emitDocumentMessage({DocumentMessage(error)});
}

void AbstractView::emitDocumentMessage(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings)
{
    if (model())
        model()->d->setDocumentMessages(errors, warnings);
}

void AbstractView::emitCustomNotification(const QString &identifier)
{
    emitCustomNotification(identifier, QList<ModelNode>());
}

void AbstractView::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList)
{
    emitCustomNotification(identifier, nodeList, QList<QVariant>());
}

void AbstractView::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    if (model())
        model()->d->notifyCustomNotification(this, identifier, nodeList, data);
}

void AbstractView::emitInstancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancePropertyChange(propertyList);
}

void AbstractView::emitInstanceErrorChange(const QVector<qint32> &instanceIds)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstanceErrorChange(instanceIds);
}

void AbstractView::emitInstancesCompleted(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesCompleted(nodeVector);
}

void AbstractView::emitInstanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesInformationsChange(informationChangeHash);
}

void AbstractView::emitInstancesRenderImageChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesRenderImageChanged(nodeVector);
}

void AbstractView::emitInstancesPreviewImageChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesPreviewImageChanged(nodeVector);
}

void AbstractView::emitInstancesChildrenChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesChildrenChanged(nodeVector);
}

void AbstractView::emitRewriterBeginTransaction()
{
    if (model())
        model()->d->notifyRewriterBeginTransaction();
}

void AbstractView::emitInstanceToken(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    if (nodeInstanceView())
        model()->d->notifyInstanceToken(token, number, nodeVector);
}

void AbstractView::emitRenderImage3DChanged(const QImage &image)
{
    if (model())
        model()->d->notifyRenderImage3DChanged(image);
}

void AbstractView::emitUpdateActiveScene3D(const QVariantMap &sceneState)
{
    if (model())
        model()->d->notifyUpdateActiveScene3D(sceneState);
}

void AbstractView::emitModelNodelPreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    if (model())
        model()->d->notifyModelNodePreviewPixmapChanged(node, pixmap);
}

void AbstractView::emitImport3DSupportChanged(const QVariantMap &supportMap)
{
    if (model())
        model()->d->notifyImport3DSupportChanged(supportMap);
}

void AbstractView::emitNodeAtPosResult(const ModelNode &modelNode, const QVector3D &pos3d)
{
    if (model())
        model()->d->notifyNodeAtPosResult(modelNode, pos3d);
}

void AbstractView::emitView3DAction(View3DActionType type, const QVariant &value)
{
    if (model())
        model()->d->notifyView3DAction(type, value);
}

void AbstractView::emitRewriterEndTransaction()
{
    if (model())
        model()->d->notifyRewriterEndTransaction();
}

void AbstractView::setCurrentStateNode(const ModelNode &node)
{
    Internal::WriteLocker locker(m_model.data());
    if (model())
        model()->d->notifyCurrentStateChanged(node);
}

void AbstractView::changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion)
{
    Internal::WriteLocker locker(m_model.data());

    m_model.data()->d->changeRootNodeType(type, majorVersion, minorVersion);
}

// Creates material library if it doesn't exist and moves any existing materials into it.
void AbstractView::ensureMaterialLibraryNode()
{
    ModelNode matLib = modelNodeForId(Constants::MATERIAL_LIB_ID);
    if (matLib.isValid()
            || (!rootModelNode().metaInfo().isQtQuick3DNode()
                && !rootModelNode().metaInfo().isQtQuickItem())) {
        return;
    }

    executeInTransaction(__FUNCTION__, [&] {
        // Create material library node
        auto nodeType = rootModelNode().metaInfo().isQtQuick3DNode()
                            ? model()->qtQuick3DNodeMetaInfo()
                            : model()->qtQuickItemMetaInfo();
        matLib = createModelNode(nodeType.typeName(), nodeType.majorVersion(), nodeType.minorVersion());

        matLib.setIdWithoutRefactoring(Constants::MATERIAL_LIB_ID);
        rootModelNode().defaultNodeListProperty().reparentHere(matLib);
    });

    // Do the material reparentings in different transaction to work around issue QDS-8094
    executeInTransaction(__FUNCTION__, [&] {
        const QList<ModelNode> materials = rootModelNode().subModelNodesOfType(
            model()->qtQuick3DMaterialMetaInfo());
        if (!materials.isEmpty()) {
            // Move all materials to under material library node
            for (const ModelNode &node : materials) {
                // If material has no name, set name to id
                QString matName = node.variantProperty("objectName").value().toString();
                if (matName.isEmpty()) {
                    VariantProperty objNameProp = node.variantProperty("objectName");
                    objNameProp.setValue(node.id());
                }

                matLib.defaultNodeListProperty().reparentHere(node);
            }
        }
    });
}

// Returns ModelNode for project's material library if it exists.
ModelNode AbstractView::materialLibraryNode()
{
    return modelNodeForId(Constants::MATERIAL_LIB_ID);
}

ModelNode AbstractView::active3DSceneNode()
{
    auto activeSceneAux = rootModelNode().auxiliaryData(active3dSceneProperty);
    if (activeSceneAux) {
        int activeScene = activeSceneAux->toInt();

        if (hasModelNodeForInternalId(activeScene))
            return modelNodeForInternalId(activeScene);
    }

    return {};
}

// Assigns given material to a 3D model.
// The assigned material is also inserted into material library if not already there.
// If given material is not valid, first existing material from material library is used,
// or if material library is empty, a new material is created.
// This function should be called only from inside a transaction, as it potentially does many
// changes to model.
void AbstractView::assignMaterialTo3dModel(const ModelNode &modelNode, const ModelNode &materialNode)
{
    QTC_ASSERT(modelNode.isValid() && modelNode.metaInfo().isQtQuick3DModel(), return );

    ModelNode matLib = materialLibraryNode();

    if (!matLib.isValid())
        return;

    ModelNode newMaterialNode;

    if (materialNode.isValid() && materialNode.metaInfo().isQtQuick3DMaterial()) {
        newMaterialNode = materialNode;
    } else {
        const QList<ModelNode> materials = matLib.directSubModelNodes();
        if (materials.size() > 0) {
            for (const ModelNode &mat : materials) {
                if (mat.metaInfo().isQtQuick3DMaterial()) {
                    newMaterialNode = mat;
                    break;
                }
            }
        }

        // if no valid material, create a new default material
        if (!newMaterialNode.isValid()) {
            NodeMetaInfo metaInfo = model()->qtQuick3DDefaultMaterialMetaInfo();
            newMaterialNode = createModelNode("QtQuick3D.DefaultMaterial", metaInfo.majorVersion(),
                                              metaInfo.minorVersion());
            newMaterialNode.validId();
        }
    }

    QTC_ASSERT(newMaterialNode.isValid(), return);

    VariantProperty matNameProp = newMaterialNode.variantProperty("objectName");
    if (matNameProp.value().isNull())
        matNameProp.setValue("New Material");

    if (!newMaterialNode.hasParentProperty()
            || newMaterialNode.parentProperty() != matLib.defaultNodeListProperty()) {
        matLib.defaultNodeListProperty().reparentHere(newMaterialNode);
    }
    BindingProperty modelMatsProp = modelNode.bindingProperty("materials");
    modelMatsProp.setExpression(newMaterialNode.id());
}

ModelNode AbstractView::getTextureDefaultInstance(const QString &source)
{
    ModelNode matLib = materialLibraryNode();
    if (!matLib.isValid())
        return {};

    const QList <ModelNode> matLibNodes = matLib.directSubModelNodes();
    for (const ModelNode &tex : matLibNodes) {
        if (tex.isValid() && tex.metaInfo().isQtQuick3DTexture()) {
            const QList<AbstractProperty> props = tex.properties();
            if (props.size() != 1)
                continue;
            const AbstractProperty &prop = props[0];
            if (prop.name() == "source" && prop.isVariantProperty()
                    && prop.toVariantProperty().value().toString() == source) {
                return tex;
            }
        }
    }

    return {};
}


ModelNode AbstractView::currentStateNode() const
{
    if (model())
        return ModelNode(m_model.data()->d->currentStateNode(), m_model.data(), const_cast<AbstractView*>(this));

    return ModelNode();
}

QmlModelState AbstractView::currentState() const
{
    return QmlModelState(currentStateNode());
}

QmlTimeline AbstractView::currentTimeline() const
{
    if (model())
        return QmlTimeline(ModelNode(m_model.data()->d->currentTimelineNode(),
                                            m_model.data(),
                                            const_cast<AbstractView*>(this)));

    return QmlTimeline();
}

static int getMinorVersionFromImport(const Model *model)
{
    const QList<Import> imports = model->imports();
    for (const Import &import : imports) {
        if (import.isLibraryImport() && import.url() == "QtQuick") {
            const QString versionString = import.version();
            if (versionString.contains(".")) {
                const QString minorVersionString = versionString.split(".").constLast();
                return minorVersionString.toInt();
            }
        }
    }

    return -1;
}

static int getMajorVersionFromImport(const Model *model)
{
    const QList<Import> imports = model->imports();
    for (const Import &import : imports) {
        if (import.isLibraryImport() && import.url() == QStringLiteral("QtQuick")) {
            const QString versionString = import.version();
            if (versionString.contains(QStringLiteral("."))) {
                const QString majorVersionString = versionString.split(QStringLiteral(".")).constFirst();
                return majorVersionString.toInt();
            }
        }
    }

    return -1;
}

static int getMajorVersionFromNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isValid()) {
        for (const NodeMetaInfo &info :  modelNode.metaInfo().classHierarchy()) {
            if (info.typeName() == "QtQml.QtObject"
                    || info.typeName() == "QtQuick.QtObject"
                    || info.typeName() == "QtQuick.Item")
                return info.majorVersion();
        }
    }

    return 1; //default
}

static int getMinorVersionFromNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isValid()) {
        const NodeMetaInfos infos =  modelNode.metaInfo().classHierarchy();
        for (const NodeMetaInfo &info :  infos) {
            if (info.typeName() == "QtQuick.QtObject" || info.typeName() == "QtQuick.Item")
                return info.minorVersion();
        }
    }

    return 1; //default
}

int AbstractView::majorQtQuickVersion() const
{
    int majorVersionFromImport = getMajorVersionFromImport(model());
    if (majorVersionFromImport >= 0)
        return majorVersionFromImport;

    return getMajorVersionFromNode(rootModelNode());
}

int AbstractView::minorQtQuickVersion() const
{
    int minorVersionFromImport = getMinorVersionFromImport(model());
    if (minorVersionFromImport >= 0)
        return minorVersionFromImport;

    return getMinorVersionFromNode(rootModelNode());
}


} // namespace QmlDesigner
