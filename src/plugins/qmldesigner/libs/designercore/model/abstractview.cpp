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
using ModelTracing::category;

/*!
\class QmlDesigner::AbstractView
\ingroup CoreModel
\brief The AbstractView class provides an abstract interface that views and
editors can implement to be notified about model changes.

\sa QmlDesigner::WidgetQueryView(), QmlDesigner::NodeInstanceView()
*/

AbstractView::~AbstractView()
{
    NanotraceHR::Tracer tracer{"abstract view destructor", category()};
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
    NanotraceHR::Tracer tracer{"abstract view set model", category()};

    Q_ASSERT(model != nullptr);
    if (model == m_model.data())
        return;

    if (m_model)
        m_model.data()->detachView(this);

    m_model = model;
}

void AbstractView::setWidgetRegistration(WidgetRegistrationInterface *interface)
{
    NanotraceHR::Tracer tracer{"abstract view set widget registration", category()};

    m_widgetRegistration = interface;
}

void AbstractView::registerWidgetInfo()
{
    NanotraceHR::Tracer tracer{"abstract view register widget info", category()};

    if (m_widgetRegistration)
        m_widgetRegistration->registerWidgetInfo(widgetInfo());
}

void AbstractView::deregisterWidgetInfo()
{
    NanotraceHR::Tracer tracer{"abstract view deregister widget info", category()};

    if (m_widgetRegistration)
        m_widgetRegistration->deregisterWidgetInfo(widgetInfo());
}

RewriterTransaction AbstractView::beginRewriterTransaction(const QByteArray &identifier)
{
    NanotraceHR::Tracer tracer{"abstract view begin rewriter transaction",
                               category(),
                               keyValue("identifier", identifier)};

    return RewriterTransaction(this, identifier);
}

ModelNode AbstractView::createModelNode(const TypeName &typeName,
                                        const QList<QPair<PropertyName, QVariant>> &propertyList,
                                        const AuxiliaryDatas &auxPropertyList,
                                        const QString &nodeSource,
                                        ModelNode::NodeSourceType nodeSourceType,
                                        const QString &behaviorPropertyName,
                                        SL sl)
{
    NanotraceHR::Tracer _{"abstract view create model node",
                          category(),
                          keyValue("type", typeName),
                          keyValue("source location", sl)};

    return ModelNode(model()->d->createNode(typeName,
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
    NanotraceHR::Tracer tracer{"abstract view root model node const", category()};

    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), const_cast<AbstractView *>(this));
}

// Returns the root model node.
ModelNode AbstractView::rootModelNode()
{
    NanotraceHR::Tracer tracer{"abstract view root model node", category()};

    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), this);
}

// Sets the reference to a model to a null pointer.
void AbstractView::removeModel()
{
    NanotraceHR::Tracer tracer{"abstract view remove model", category()};

    m_model.clear();
}

WidgetInfo AbstractView::createWidgetInfo(QWidget *widget,
                                          const QString &uniqueId,
                                          WidgetInfo::PlacementHint placementHint,
                                          const QString &tabName,
                                          const QString &feedbackDisplayName,
                                          DesignerWidgetFlags widgetFlags,
                                          const QString &parentId)
{
    NanotraceHR::Tracer tracer{"abstract view create widget info", category()};

    WidgetInfo widgetInfo;
    widgetInfo.widget = widget;
    widgetInfo.uniqueId = uniqueId;
    widgetInfo.placementHint = placementHint;
    widgetInfo.tabName = tabName;
    widgetInfo.feedbackDisplayName = feedbackDisplayName;
    widgetInfo.widgetFlags = widgetFlags;
    widgetInfo.parentId = parentId;

    return widgetInfo;
}

bool AbstractView::isAttached() const
{
    NanotraceHR::Tracer tracer{"abstract view is attached", category()};

    return model();
}

/*!
Called if a view is being attached to \a model.
The default implementation is setting the reference of the model to the view.
\sa Model::attachView()
*/
void AbstractView::modelAttached(Model *model)
{
    NanotraceHR::Tracer tracer{"abstract view model attached",
                               ModelTracing::category(),
                               keyValue("widget id", widgetInfo().uniqueId)};

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
    NanotraceHR::Tracer tracer{"abstract view model about to be detached",
                               ModelTracing::category(),
                               keyValue("widget id", widgetInfo().uniqueId)};
    removeModel();
}

void AbstractView::refreshMetaInfos(const TypeIds &) {}

void AbstractView::exportedTypeNamesChanged(const ExportedTypeNames &, const ExportedTypeNames &) {}

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

void AbstractView::rootNodeTypeChanged(const QString & /*type*/) {}

void AbstractView::nodeTypeChanged(const ModelNode & /*node*/, const TypeName & /*type*/) {}

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
    NanotraceHR::Tracer tracer{"abstract view to model node list", ModelTracing::category()};

    return QmlDesigner::toModelNodeList(nodes, m_model, const_cast<AbstractView *>(this));
}

QList<ModelNode> toModelNodeList(Utils::span<const Internal::InternalNode::Pointer> nodes,
                                 Model *model,
                                 AbstractView *view)
{
    NanotraceHR::Tracer tracer{"abstract view to model node list", ModelTracing::category()};

    QList<ModelNode> newNodeList;
    newNodeList.reserve(std::ssize(nodes));
    for (const Internal::InternalNode::Pointer &node : nodes)
        newNodeList.emplace_back(node, model, view);

    return newNodeList;
}

/*!
    Sets the list of nodes to the actual selected nodes specified by
    \a selectedNodeList if the node or its ancestors are not locked.
*/
void AbstractView::setSelectedModelNodes(const QList<ModelNode> &selectedNodeList)
{
    NanotraceHR::Tracer tracer{"abstract view set selected model nodes", ModelTracing::category()};

    model()->setSelectedModelNodes(selectedNodeList);
}

void AbstractView::setSelectedModelNode(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"abstract view set selected model node", ModelTracing::category()};

    if (ModelUtils::isThisOrAncestorLocked(modelNode)) {
        clearSelectedModelNodes();
        return;
    }
    setSelectedModelNodes({modelNode});
}

void AbstractView::clearSelectedModelNodes()
{
    NanotraceHR::Tracer tracer{"abstract view clear selected model nodes", ModelTracing::category()};

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
    NanotraceHR::Tracer tracer{"abstract view is selected model node", ModelTracing::category()};

    return model()->d->selectedNodes().contains(modelNode.internalNode());
}

QList<ModelNode> AbstractView::selectedModelNodes() const
{
    NanotraceHR::Tracer tracer{"abstract view selected model nodes", ModelTracing::category()};

    return toModelNodeList(model()->d->selectedNodes());
}

ModelNode AbstractView::firstSelectedModelNode() const
{
    NanotraceHR::Tracer tracer{"abstract view first selected model node", ModelTracing::category()};

    if (hasSelectedModelNodes())
        return ModelNode(model()->d->selectedNodes().front(), model(), this);

    return ModelNode();
}

ModelNode AbstractView::singleSelectedModelNode() const
{
    NanotraceHR::Tracer tracer{"abstract view single selected model node", ModelTracing::category()};

    if (hasSingleSelectedModelNode())
        return ModelNode(model()->d->selectedNodes().front(), model(), this);

    return ModelNode();
}

void AbstractView::selectModelNode(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"abstract view select model node", ModelTracing::category()};

    QTC_ASSERT(modelNode.isInHierarchy(), return);
    model()->d->selectNode(modelNode.internalNode());
}

void AbstractView::deselectModelNode(const ModelNode &node)
{
    model()->d->deselectNode(node.internalNode());
}

ModelNode AbstractView::modelNodeForId(const QString &id)
{
    NanotraceHR::Tracer tracer{"abstract view model node for id", ModelTracing::category()};

    return ModelNode(model()->d->nodeForId(id), model(), this);
}

bool AbstractView::hasId(const QString &id) const
{
    return model()->hasId(id);
}

ModelNode AbstractView::modelNodeForInternalId(qint32 internalId) const
{
    NanotraceHR::Tracer tracer{"abstract view model node for internal id", ModelTracing::category()};

    return ModelNode(model()->d->nodeForInternalId(internalId), model(), this);
}

bool AbstractView::hasModelNodeForInternalId(qint32 internalId) const
{
    NanotraceHR::Tracer tracer{"abstract view has model node for internal id",
                               ModelTracing::category()};

    return model()->d->hasNodeForInternalId(internalId);
}

const AbstractView *AbstractView::nodeInstanceView() const
{
    NanotraceHR::Tracer tracer{"abstract view node instance view", ModelTracing::category()};

    if (isAttached())
        return model()->d->nodeInstanceView();

    return nullptr;
}

RewriterView *AbstractView::rewriterView() const
{
    NanotraceHR::Tracer tracer{"abstract view rewriter view", ModelTracing::category()};

    if (model())
        return model()->d->rewriterView();

    return nullptr;
}

void AbstractView::resetView()
{
    NanotraceHR::Tracer tracer{"abstract view reset view", ModelTracing::category()};

    if (!model())
        return;

    Model *currentModel = model();

    currentModel->detachView(this);
    currentModel->attachView(this);
}

void AbstractView::resetPuppet()
{
    NanotraceHR::Tracer tracer{"abstract view reset puppet", ModelTracing::category()};

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
    NanotraceHR::Tracer tracer{"abstract view disable widget", ModelTracing::category()};

    if (hasWidget() && widgetInfo().widgetFlags == DesignerWidgetFlags::DisableOnError)
        widgetInfo().widget->setEnabled(false);
}

void AbstractView::enableWidget()
{
    NanotraceHR::Tracer tracer{"abstract view enable widget", ModelTracing::category()};

    if (hasWidget()) {
        if (auto info = widgetInfo(); info.widgetFlags == DesignerWidgetFlags::DisableOnError)
            info.widget->setEnabled(true);
    }
}

QString AbstractView::contextHelpId() const
{
    NanotraceHR::Tracer tracer{"abstract view context help id", ModelTracing::category()};

    QString id = const_cast<AbstractView *>(this)->widgetInfo().uniqueId;

    auto selectedNodes = selectedModelNodes();

    if (!selectedNodes.isEmpty()) {
        const auto nodeId = selectedNodes.first().exportedTypeName().name.toQString();
        id += " " + nodeId;
    }

    return id;
}

bool AbstractView::executeInTransaction(const QByteArray &identifier, const OperationBlock &lambda)
{
    NanotraceHR::Tracer tracer{"abstract view execute in transaction", ModelTracing::category()};

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
    NanotraceHR::Tracer tracer{"abstract view all model nodes", ModelTracing::category()};

    QTC_ASSERT(model(), return {});
    return toModelNodeList(model()->d->allNodesOrdered());
}

QList<ModelNode> AbstractView::allModelNodesUnordered() const
{
    NanotraceHR::Tracer tracer{"abstract view all model nodes unordered", ModelTracing::category()};

    return toModelNodeList(model()->d->allNodesUnordered());
}

QList<ModelNode> AbstractView::allModelNodesOfType(const NodeMetaInfo &type) const
{
    NanotraceHR::Tracer tracer{"abstract view all model nodes of type", ModelTracing::category()};

    return Utils::filtered(allModelNodes(),
                           [&](const ModelNode &node) { return node.metaInfo().isBasedOn(type); });
}

void AbstractView::setCurrentStateNode(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"abstract view set current state node", ModelTracing::category()};

    if (m_model)
        m_model->setCurrentStateNode(node);
}

void AbstractView::changeRootNodeType(const TypeName &type)
{
    NanotraceHR::Tracer tracer{"abstract view change root node type", ModelTracing::category()};

    Internal::WriteLocker locker(m_model.data());

    m_model.data()->d->changeRootNodeType(type);
}

ModelNode AbstractView::currentStateNode() const
{
    NanotraceHR::Tracer tracer{"abstract view current state node", ModelTracing::category()};

    if (m_model)
        return m_model->currentStateNode(const_cast<AbstractView *>(this));

    return {};
}

ModelNode AbstractView::currentTimelineNode() const
{
    NanotraceHR::Tracer tracer{"abstract view current timeline node", ModelTracing::category()};

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

int AbstractView::majorQtQuickVersion() const
{
    NanotraceHR::Tracer tracer{"abstract view major qt quick version", ModelTracing::category()};

    int majorVersionFromImport = getMajorVersionFromImport(model());
    if (majorVersionFromImport >= 0)
        return majorVersionFromImport;

    return -1;
}

int AbstractView::minorQtQuickVersion() const
{
    NanotraceHR::Tracer tracer{"abstract view minor qt quick version", ModelTracing::category()};

    int minorVersionFromImport = getMinorVersionFromImport(model());
    if (minorVersionFromImport >= 0)
        return minorVersionFromImport;

    return -1;
}

AbstractViewAction::AbstractViewAction(AbstractView &view)
    : m_view{view}
{
    connect(this, &QAction::toggled, [&](bool isChecked) {
        emit viewCheckedChanged(isChecked, view);
    });
}

} // namespace QmlDesigner
