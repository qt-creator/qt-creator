// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractview.h"

#include "bindingproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "rewritertransaction.h"
#include "variantproperty.h"

#include <modelutils.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/span.h>

#include <QWidget>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;
using NanotraceHR::keyValue;
using namespace Qt::StringLiterals;

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
#ifdef QDS_USE_PROJECTSTORAGE
    return createModelNode(typeName, -1, -1);
#else
    const NodeMetaInfo metaInfo = model()->metaInfo(typeName);
    return createModelNode(typeName, metaInfo.majorVersion(), metaInfo.minorVersion());
#endif
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
    return ModelNode(model()->d->createNode(typeName, majorVersion, minorVersion, propertyList,
                                            auxPropertyList, nodeSource, nodeSourceType,
                                            behaviorPropertyName), model(), this);
}

ModelNode AbstractView::createModelNode(const TypeName &typeName,
                                        const QList<QPair<PropertyName, QVariant>> &propertyList,
                                        const AuxiliaryDatas &auxPropertyList,
                                        const QString &nodeSource,
                                        ModelNode::NodeSourceType nodeSourceType,
                                        const QString &behaviorPropertyName)
{
    return ModelNode(model()->d->createNode(typeName,
                                            -1,
                                            -1,
                                            propertyList,
                                            auxPropertyList,
                                            nodeSource,
                                            nodeSourceType,
                                            behaviorPropertyName),
                     model(),
                     this);
}

// Returns the constant root model node.
ModelNode AbstractView::rootModelNode() const
{
    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), const_cast<AbstractView *>(this));
}

// Returns the root model node.
ModelNode AbstractView::rootModelNode()
{
    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), this);
}

// Sets the reference to a model to a null pointer.
void AbstractView::removeModel()
{
    m_model.clear();
}

WidgetInfo AbstractView::createWidgetInfo(QWidget *widget,
                                          const QString &uniqueId,
                                          WidgetInfo::PlacementHint placementHint,
                                          const QString &tabName,
                                          const QString &feedbackDisplayName,
                                          DesignerWidgetFlags widgetFlags)
{
    WidgetInfo widgetInfo;

    widgetInfo.widget = widget;
    widgetInfo.uniqueId = uniqueId;
    widgetInfo.placementHint = placementHint;
    widgetInfo.tabName = tabName;
    widgetInfo.feedbackDisplayName = feedbackDisplayName;
    widgetInfo.widgetFlags = widgetFlags;

    return widgetInfo;
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

void AbstractView::refreshMetaInfos(const TypeIds &) {}

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

void AbstractView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
}

void AbstractView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/,
                               PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &/*propertyList*/)
{
}

void AbstractView::propertiesRemoved(const QList<AbstractProperty> &/*propertyList*/)
{
}

void AbstractView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{
}

void AbstractView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/,
                                           const NodeAbstractProperty &/*oldPropertyParent*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/,
                                  const NodeAbstractProperty &/*oldPropertyParent*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::nodeIdChanged(const ModelNode &/*node*/, const QString &/*newId*/, const QString &/*oldId*/)
{
}

void AbstractView::variantPropertiesChanged(const QList<VariantProperty>& /*propertyList*/,
                                            PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::bindingPropertiesAboutToBeChanged(const QList<BindingProperty> &) {}

void AbstractView::bindingPropertiesChanged(const QList<BindingProperty>& /*propertyList*/,
                                            PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &/*propertyList*/,
                                                  PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::signalDeclarationPropertiesChanged(const QVector<SignalDeclarationProperty> &/*propertyList*/,
                                                      PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
}

void AbstractView::nodeTypeChanged(const ModelNode & /*node*/, const TypeName & /*type*/, int /*majorVersion*/, int /*minorVersion*/)
{

}

void AbstractView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
{
}

void AbstractView::possibleImportsChanged(const Imports &/*possibleImports*/)
{
}

void AbstractView::usedImportsChanged(const Imports &/*usedImports*/)
{
}

void AbstractView::auxiliaryDataChanged(const ModelNode &/*node*/,
                                        AuxiliaryDataKeyView /*key*/,
                                        const QVariant &/*data*/)
{
}

void AbstractView::customNotification(const AbstractView * /*view*/, const QString & /*identifier*/,
                                      const QList<ModelNode> & /*nodeList*/, const QList<QVariant> & /*data*/)
{
}

void AbstractView::customNotification(const CustomNotificationPackage &) {}

void AbstractView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{
}

void AbstractView::documentMessagesChanged(const QList<DocumentMessage> &/*errors*/, const QList<DocumentMessage> &/*warnings*/)
{
}

void AbstractView::currentTimelineChanged(const ModelNode &/*node*/)
{
}

void AbstractView::renderImage3DChanged(const QImage &/*image*/)
{
}

void AbstractView::updateActiveScene3D(const QVariantMap &/*sceneState*/)
{
}

void AbstractView::updateImport3DSupport(const QVariantMap &/*supportMap*/)
{
}

// a Quick3DNode that is picked at the requested view position in the 3D Editor and the 3D scene
// position of the requested view position.
void AbstractView::nodeAtPosReady(const ModelNode &/*modelNode*/, const QVector3D &/*pos3d*/) {}

void AbstractView::modelNodePreviewPixmapChanged(const ModelNode & /*node*/,
                                                 const QPixmap & /*pixmap*/,
                                                 const QByteArray & /*requestId*/)
{
}

void AbstractView::view3DAction(View3DActionType, const QVariant &) {}

void AbstractView::dragStarted(QMimeData * /*mimeData*/) {}
void AbstractView::dragEnded() {}

QList<ModelNode> AbstractView::toModelNodeList(Utils::span<const Internal::InternalNode::Pointer> nodes) const
{
    return QmlDesigner::toModelNodeList(nodes, m_model, const_cast<AbstractView *>(this));
}

QList<ModelNode> toModelNodeList(Utils::span<const Internal::InternalNode::Pointer> nodes,
                                 Model *model,
                                 AbstractView *view)
{
    QList<ModelNode> newNodeList;
    newNodeList.reserve(std::ssize(nodes));
    for (const Internal::InternalNode::Pointer &node : nodes)
        newNodeList.emplace_back(node, model, view);

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
    model()->setSelectedModelNodes(selectedNodeList);
}

void AbstractView::setSelectedModelNode(const ModelNode &modelNode)
{
    if (ModelUtils::isThisOrAncestorLocked(modelNode)) {
        clearSelectedModelNodes();
        return;
    }
    setSelectedModelNodes({modelNode});
}

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
    return model()->d->selectedNodes().size() == 1;
}

bool AbstractView::isSelectedModelNode(const ModelNode &modelNode) const
{
    return model()->d->selectedNodes().contains(modelNode.internalNode());
}

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

void AbstractView::selectModelNode(const ModelNode &modelNode)
{
    QTC_ASSERT(modelNode.isInHierarchy(), return);
    model()->d->selectNode(modelNode.internalNode());
}

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

const AbstractView *AbstractView::nodeInstanceView() const
{
    if (isAttached())
        return model()->d->nodeInstanceView();

    return nullptr;
}

RewriterView *AbstractView::rewriterView() const
{
    if (model())
        return model()->d->rewriterView();

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
    if (hasWidget()) {
        if (auto info = widgetInfo(); info.widgetFlags == DesignerWidgetFlags::DisableOnError)
            info.widget->setEnabled(true);
    }
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

bool AbstractView::executeInTransaction(const QByteArray &identifier, const OperationBlock &lambda)
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

QList<ModelNode> AbstractView::allModelNodes() const
{
    QTC_ASSERT(model(), return {});
    return toModelNodeList(model()->d->allNodesOrdered());
}

QList<ModelNode> AbstractView::allModelNodesUnordered() const
{
    return toModelNodeList(model()->d->allNodesUnordered());
}

QList<ModelNode> AbstractView::allModelNodesOfType(const NodeMetaInfo &type) const
{
    return Utils::filtered(allModelNodes(),
                           [&](const ModelNode &node) { return node.metaInfo().isBasedOn(type); });
}

void AbstractView::setCurrentStateNode(const ModelNode &node)
{
    if (m_model)
        m_model->setCurrentStateNode(node);
}

void AbstractView::changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion)
{
    Internal::WriteLocker locker(m_model.data());

    m_model.data()->d->changeRootNodeType(type, majorVersion, minorVersion);
}

ModelNode AbstractView::currentStateNode() const
{
    if (m_model)
        return m_model->currentStateNode(const_cast<AbstractView *>(this));

    return {};
}

ModelNode AbstractView::currentTimelineNode() const
{
    if (isAttached()) {
        return ModelNode(m_model.data()->d->currentTimelineNode(),
                         m_model.data(),
                         const_cast<AbstractView *>(this));
    }

    return {};
}

static int getMinorVersionFromImport(const Model *model)
{
    const Imports &imports = model->imports();

    auto found = std::ranges::find(imports, "QtQuick"_L1, &Import::url);

    if (found != imports.end())
        return found->minorVersion();

    return -1;
}

static int getMajorVersionFromImport(const Model *model)
{
    const Imports &imports = model->imports();

    auto found = std::ranges::find(imports, "QtQuick"_L1, &Import::url);

    if (found != imports.end())
        return found->majorVersion();

    return -1;
}

#ifndef QDS_USE_PROJECTSTORAGE
static int getMajorVersionFromNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isValid()) {
        for (const NodeMetaInfo &info : modelNode.metaInfo().selfAndPrototypes()) {
            if (info.isQtObject() || info.isQtQuickItem())
                return info.majorVersion();
        }
    }

    return 1; // default
}

static int getMinorVersionFromNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isValid()) {
        const NodeMetaInfos infos = modelNode.metaInfo().selfAndPrototypes();
        for (const NodeMetaInfo &info :  infos) {
            if (info.isQtObject() || info.isQtQuickItem())
                return info.minorVersion();
        }
    }

    return 1; // default
}
#endif

int AbstractView::majorQtQuickVersion() const
{
    int majorVersionFromImport = getMajorVersionFromImport(model());
    if (majorVersionFromImport >= 0)
        return majorVersionFromImport;

#ifdef QDS_USE_PROJECTSTORAGE
    return -1;
#else
    return getMajorVersionFromNode(rootModelNode());
#endif
}

int AbstractView::minorQtQuickVersion() const
{
    int minorVersionFromImport = getMinorVersionFromImport(model());
    if (minorVersionFromImport >= 0)
        return minorVersionFromImport;

#ifdef QDS_USE_PROJECTSTORAGE
    return -1;
#else
    return getMinorVersionFromNode(rootModelNode());
#endif
}

AbstractViewAction::AbstractViewAction(AbstractView &view)
    : m_view{view}
{
    connect(this, &QAction::toggled, [&](bool isChecked) {
        emit viewCheckedChanged(isChecked, view);
    });
}

} // namespace QmlDesigner
