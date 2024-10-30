// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "model.h"
#include "internalnode_p.h"
#include "model_p.h"

#include <modelnode.h>
#include <modelutils.h>

#include "abstractview.h"
#include "internalbindingproperty.h"
#include "internalnodeabstractproperty.h"
#include "internalnodelistproperty.h"
#include "internalnodeproperty.h"
#include "internalproperty.h"
#include "internalsignalhandlerproperty.h"
#include "internalvariantproperty.h"
#include "itemlibraryentry.h"
#include "sourcepathstorage/sourcepath.h"
#include "sourcepathstorage/sourcepathcache.h"
#ifndef QDS_USE_PROJECTSTORAGE
#  include "metainfo.h"
#endif
#include "nodemetainfo.h"

#include "abstractproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "rewriterview.h"
#include "rewritingexception.h"
#include "signalhandlerproperty.h"
#include "variantproperty.h"

#include <designeralgorithm.h>
#include <predicate.h>
#include <uniquename.h>

#include <projectstorage/projectstorage.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qmldesignerutils/version.h>
#include <utils/algorithm.h>

#include <QDrag>
#include <QFileInfo>
#include <QHashIterator>
#include <QPointer>
#include <QRegularExpression>
#include <qcompilerdetection.h>

#include <algorithm>
#include <functional>

/*!
\defgroup CoreModel
*/
/*!
\class QmlDesigner::Model
\ingroup CoreModel
\brief This is the facade for the abstract model data.
 All write access is running through this interface

The Model is the central place to access a qml files data (see e.g. rootNode() ) and meta data (see metaInfo() ).
Components that want to be informed about changes in the model can register a subclass of AbstractView via attachView().

\see QmlDesigner::ModelNode, QmlDesigner::AbstractProperty, QmlDesigner::AbstractView
*/

namespace QmlDesigner {

using NanotraceHR::keyValue;

namespace Internal {

ModelPrivate::ModelPrivate(Model *model,
                           ProjectStorageDependencies projectStorageDependencies,
                           const TypeName &typeName,
                           int major,
                           int minor,
                           Model *metaInfoProxyModel,
                           std::unique_ptr<ModelResourceManagementInterface> resourceManagement)
    : projectStorage{&projectStorageDependencies.storage}
    , pathCache{&projectStorageDependencies.cache}
    , m_model{model}
    , m_resourceManagement{std::move(resourceManagement)}
{
    m_metaInfoProxyModel = metaInfoProxyModel;

    changeImports({Import::createLibraryImport({"QtQuick"})}, {});

    m_rootInternalNode = createNode(
        typeName, major, minor, {}, {}, {}, ModelNode::NodeWithoutSource, {}, true);

    m_currentStateNode = m_rootInternalNode;
    m_currentTimelineNode = m_rootInternalNode;

    if constexpr (useProjectStorage())
        projectStorage->addObserver(this);
}

ModelPrivate::ModelPrivate(Model *model,
                           ProjectStorageDependencies projectStorageDependencies,
                           Utils::SmallStringView typeName,
                           Imports imports,
                           const QUrl &fileUrl,
                           std::unique_ptr<ModelResourceManagementInterface> resourceManagement)
    : projectStorage{&projectStorageDependencies.storage}
    , pathCache{&projectStorageDependencies.cache}
    , m_model{model}
    , m_resourceManagement{std::move(resourceManagement)}
{
    setFileUrl(fileUrl);
    changeImports(std::move(imports), {});

    m_rootInternalNode = createNode(
        typeName, -1, -1, {}, {}, {}, ModelNode::NodeWithoutSource, {}, true);

    m_currentStateNode = m_rootInternalNode;
    m_currentTimelineNode = m_rootInternalNode;

    if constexpr (useProjectStorage())
        projectStorage->addObserver(this);
}

ModelPrivate::ModelPrivate(Model *model,
                           const TypeName &typeName,
                           int major,
                           int minor,
                           Model *metaInfoProxyModel,
                           std::unique_ptr<ModelResourceManagementInterface> resourceManagement)
    : m_model(model)
    , m_resourceManagement{std::move(resourceManagement)}
{
    m_metaInfoProxyModel = metaInfoProxyModel;

    m_rootInternalNode = createNode(
        typeName, major, minor, {}, {}, {}, ModelNode::NodeWithoutSource, {}, true);

    m_currentStateNode = m_rootInternalNode;
    m_currentTimelineNode = m_rootInternalNode;
}

ModelPrivate::~ModelPrivate()
{
    removeNode(rootNode());
    if constexpr (useProjectStorage())
        projectStorage->removeObserver(this);
};

void ModelPrivate::detachAllViews()
{
    auto tracer = traceToken.begin("detach all views");

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
        detachView(view.data(), true);

    m_viewList.clear();

    if (m_nodeInstanceView) {
        m_nodeInstanceView->modelAboutToBeDetached(m_model);
        m_nodeInstanceView.clear();
    }

    if (m_rewriterView) {
        m_rewriterView->modelAboutToBeDetached(m_model);
        m_rewriterView.clear();
    }
}

namespace {
Storage::Imports createStorageImports(const Imports &imports,
                                      Utils::SmallStringView localDirectoryPath,
                                      ProjectStorageType &projectStorage,
                                      SourceId fileId)
{
    using Storage::ModuleKind;
    Storage::Imports storageImports;
    storageImports.reserve(Utils::usize(imports) + 1);

    for (const Import &import : imports) {
        auto moduleKind = import.isLibraryImport() ? ModuleKind::QmlLibrary : ModuleKind::PathLibrary;
        auto moduleId = projectStorage.moduleId(Utils::SmallString{import.url()}, moduleKind);
        storageImports.emplace_back(moduleId, import.majorVersion(), import.minorVersion(), fileId);
    }

    auto localDirectoryModuleId = projectStorage.moduleId(localDirectoryPath, ModuleKind::PathLibrary);

    storageImports.emplace_back(localDirectoryModuleId, Storage::Version{}, fileId);

    return storageImports;
}

} // namespace

void ModelPrivate::changeImports(Imports toBeAddedImports, Imports toBeRemovedImports)
{
    auto tracer = traceToken.begin("change imports");

    std::sort(toBeAddedImports.begin(), toBeAddedImports.end());
    std::sort(toBeRemovedImports.begin(), toBeRemovedImports.end());

    Imports removedImports = set_intersection(m_imports, toBeRemovedImports);
    m_imports = set_difference(m_imports, removedImports);

    Imports allNewAddedImports = set_strict_difference(toBeAddedImports, m_imports);
    Imports importWithoutAddedImport = set_difference(m_imports, allNewAddedImports);

    m_imports = set_union(importWithoutAddedImport, allNewAddedImports);

    if (!removedImports.isEmpty() || !allNewAddedImports.isEmpty()) {
        if (useProjectStorage()) {
            auto imports = createStorageImports(m_imports, m_localPath, *projectStorage, m_sourceId);
            projectStorage->synchronizeDocumentImports(std::move(imports), m_sourceId);
        }
        notifyImportsChanged(allNewAddedImports, removedImports);
    }
}

void ModelPrivate::notifyImportsChanged(const Imports &addedImports, const Imports &removedImports)
{
    bool resetModel = false;
    QString description;

    try {
        if (m_rewriterView)
            m_rewriterView->importsChanged(addedImports, removedImports);
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    m_nodeMetaInfoCache.clear();

    if (nodeInstanceView())
        nodeInstanceView()->importsChanged(addedImports, removedImports);

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
        view->importsChanged(addedImports, removedImports);

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyPossibleImportsChanged(const Imports &possibleImports)
{
    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        Q_ASSERT(view != nullptr);
        view->possibleImportsChanged(possibleImports);
    }
}

void ModelPrivate::notifyUsedImportsChanged(const Imports &usedImports)
{
    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        Q_ASSERT(view != nullptr);
        view->usedImportsChanged(usedImports);
    }
}

const QUrl &ModelPrivate::fileUrl() const
{
    return m_fileUrl;
}

void ModelPrivate::setDocumentMessages(const QList<DocumentMessage> &errors,
                                       const QList<DocumentMessage> &warnings)
{
    for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
        view->documentMessagesChanged(errors, warnings);
}

void ModelPrivate::setFileUrl(const QUrl &fileUrl)
{
    auto tracer = traceToken.begin("file url");

    QUrl oldPath = m_fileUrl;

    if (oldPath != fileUrl) {
        m_fileUrl = fileUrl;
        if constexpr (useProjectStorage()) {
            auto path = fileUrl.toString(QUrl::PreferLocalFile);
            m_sourceId = pathCache->sourceId(SourcePath{path});
            auto found = std::find(path.rbegin(), path.rend(), u'/').base();
            m_localPath = Utils::PathString{QStringView{path.begin(), std::prev(found)}};
            auto imports = createStorageImports(m_imports, m_localPath, *projectStorage, m_sourceId);
            projectStorage->synchronizeDocumentImports(std::move(imports), m_sourceId);
        }

        for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
            view->fileUrlChanged(oldPath, fileUrl);
    }
}

void ModelPrivate::changeNodeType(const InternalNodePointer &node, const TypeName &typeName,
                                  int majorVersion, int minorVersion)
{
    node->typeName = typeName;
    node->majorVersion = majorVersion;
    node->minorVersion = minorVersion;

    try {
        notifyNodeTypeChanged(node, typeName, majorVersion, minorVersion);
    } catch (const RewritingException &) {
    }
}

InternalNodePointer ModelPrivate::createNode(TypeNameView typeName,
                                             int majorVersion,
                                             int minorVersion,
                                             const QList<QPair<PropertyName, QVariant>> &propertyList,
                                             const AuxiliaryDatas &auxiliaryDatas,
                                             const QString &nodeSource,
                                             ModelNode::NodeSourceType nodeSourceType,
                                             const QString &behaviorPropertyName,
                                             bool isRootNode)
{
    if (typeName.isEmpty())
        return {};

    qint32 internalId = 0;

    if (!isRootNode)
        internalId = m_internalIdCounter++;

    auto newNode = std::make_shared<InternalNode>(typeName,
                                                  majorVersion,
                                                  minorVersion,
                                                  internalId,
                                                  traceToken.tickWithFlow("create node"));

    setTypeId(newNode.get(), typeName);

    newNode->nodeSourceType = nodeSourceType;

    newNode->behaviorPropertyName = behaviorPropertyName;

    using PropertyPair = QPair<PropertyName, QVariant>;

    for (const PropertyPair &propertyPair : propertyList) {
        newNode->addVariantProperty(propertyPair.first);
        newNode->variantProperty(propertyPair.first)->setValue(propertyPair.second);
    }

    for (const auto &auxiliaryData : auxiliaryDatas)
        newNode->setAuxiliaryData(AuxiliaryDataKeyView{auxiliaryData.first}, auxiliaryData.second);

    m_nodes.push_back(newNode);
    m_internalIdNodeHash.insert(newNode->internalId, newNode);

    if (!nodeSource.isNull())
        newNode->nodeSource = nodeSource;

    notifyNodeCreated(newNode);

    if (newNode->hasProperties())
        notifyVariantPropertiesChanged(newNode,
                                       newNode->propertyNameViews(),
                                       AbstractView::PropertiesAdded);

    return newNode;
}

void ModelPrivate::removeNodeFromModel(const InternalNodePointer &node)
{
    Q_ASSERT(node);

    node->resetParentProperty();

    m_selectedInternalNodes.removeAll(node);
    if (!node->id.isEmpty())
        m_idNodeHash.remove(node->id);
    node->isValid = false;
    node->traceToken.end();
    m_nodes.removeOne(node);
    m_internalIdNodeHash.remove(node->internalId);
}

namespace {
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunneeded-internal-declaration")

std::pair<Utils::SmallStringView, Utils::SmallStringView> decomposeTypePath(Utils::SmallStringView typeName)
{
    auto found = std::find(typeName.rbegin(), typeName.rend(), '.');

    if (found == typeName.rend())
        return {{}, typeName};

    return {{typeName.begin(), std::prev(found.base())}, {found.base(), typeName.end()}};
}

QT_WARNING_POP
} // namespace

ImportedTypeNameId ModelPrivate::importedTypeNameId(Utils::SmallStringView typeName)
{
    if constexpr (useProjectStorage()) {
        auto [moduleName, shortTypeName] = decomposeTypePath(typeName);

        if (moduleName.size()) {
            QString aliasName = QString{moduleName};
            auto found = std::ranges::find(m_imports, aliasName, &Import::alias);
            if (found != m_imports.end()) {
                using Storage::ModuleKind;
                auto moduleKind = found->isLibraryImport() ? ModuleKind::QmlLibrary
                                                           : ModuleKind::PathLibrary;
                ModuleId moduleId = projectStorage->moduleId(Utils::PathString{found->url()},
                                                             moduleKind);
                ImportId importId = projectStorage->importId(
                    Storage::Import{moduleId, found->majorVersion(), found->minorVersion(), m_sourceId});
                return projectStorage->importedTypeNameId(importId, shortTypeName);
            }
        }

        return projectStorage->importedTypeNameId(m_sourceId, shortTypeName);
    }

    return ImportedTypeNameId{};
}

void ModelPrivate::setTypeId(InternalNode *node, Utils::SmallStringView typeName)
{
    if constexpr (useProjectStorage()) {
        node->importedTypeNameId = importedTypeNameId(typeName);
        node->typeId = projectStorage->typeId(node->importedTypeNameId);
    }
}

void ModelPrivate::refreshTypeId(InternalNode *node)
{
    if constexpr (useProjectStorage())
        node->typeId = projectStorage->typeId(node->importedTypeNameId);
}

void ModelPrivate::handleResourceSet(const ModelResourceSet &resourceSet)
{
    for (const ModelNode &node : resourceSet.removeModelNodes) {
        if (node)
            removeNode(node.m_internalNode);
    }

    removeProperties(toInternalProperties(resourceSet.removeProperties));

    setBindingProperties(resourceSet.setExpressions);
}

void ModelPrivate::updateModelNodeTypeIds(const TypeIds &removedTypeIds)
{
    auto nodes = m_nodes;

    std::ranges::sort(nodes, {}, &InternalNode::typeId);

    auto refeshNodeTypeId = [&](auto &node) { refreshTypeId(node.get()); };

    Utils::set_greedy_intersection(nodes, removedTypeIds, refeshNodeTypeId, {}, &InternalNode::typeId);
}

void ModelPrivate::removedTypeIds(const TypeIds &removedTypeIds)
{
    updateModelNodeTypeIds(removedTypeIds);

    notifyNodeInstanceViewLast([&](AbstractView *view) { view->refreshMetaInfos(removedTypeIds); });
}

void ModelPrivate::exportedTypesChanged()
{
    for (auto &node : m_nodes) {
        if (!node->typeId)
            refreshTypeId(node.get());
    }
}

void ModelPrivate::removeAllSubNodes(const InternalNodePointer &node)
{
    for (const InternalNodePointer &subNode : node->allSubNodes())
        removeNodeFromModel(subNode);
}

void ModelPrivate::removeNodeAndRelatedResources(const InternalNodePointer &node)
{
    if (m_resourceManagement) {
        handleResourceSet(
            m_resourceManagement->removeNodes({ModelNode{node, m_model, nullptr}}, m_model));
    } else {
        removeNode(node);
    }
}

void ModelPrivate::removeNode(const InternalNodePointer &node)
{
    AbstractView::PropertyChangeFlags propertyChangeFlags = AbstractView::NoAdditionalChanges;

    notifyNodeAboutToBeRemoved(node);

    auto oldParentProperty = node->parentProperty();

    removeAllSubNodes(node);
    removeNodeFromModel(node);

    InternalNodePointer parentNode;
    PropertyNameView parentPropertyName;
    if (oldParentProperty) {
        parentNode = oldParentProperty->propertyOwner();
        parentPropertyName = oldParentProperty->name();
    }

    if (oldParentProperty && oldParentProperty->isEmpty()) {
        removePropertyWithoutNotification(oldParentProperty.get());

        propertyChangeFlags |= AbstractView::EmptyPropertiesRemoved;
    }

    notifyNodeRemoved(node, parentNode, parentPropertyName, propertyChangeFlags);
}

InternalNodePointer ModelPrivate::rootNode() const
{
    return m_rootInternalNode;
}

#ifndef QDS_USE_PROJECTSTORAGE
MetaInfo ModelPrivate::metaInfo() const
{
    return m_metaInfo;
}

void ModelPrivate::setMetaInfo(const MetaInfo &metaInfo)
{
    m_metaInfo = metaInfo;
}
#endif

void ModelPrivate::changeNodeId(const InternalNodePointer &node, const QString &id)
{
    using namespace NanotraceHR::Literals;

    const QString oldId = node->id;

    node->id = id;
    node->traceToken.tick("id", std::forward_as_tuple("id", id));
    if (!oldId.isEmpty())
        m_idNodeHash.remove(oldId);
    if (!id.isEmpty())
        m_idNodeHash.insert(id, node);

    try {
        notifyNodeIdChanged(node, id, oldId);
    } catch (const RewritingException &) {
    }
}

bool ModelPrivate::propertyNameIsValid(PropertyNameView propertyName)
{
    if (propertyName.isEmpty())
        return false;

    if (propertyName == "id")
        return false;

    return true;
}

template<typename Callable>
void ModelPrivate::notifyNodeInstanceViewLast(Callable call)
{
    bool resetModel = false;
    QString description;

    try {
        if (m_rewriterView && !m_rewriterView->isBlockingNotifications())
            call(m_rewriterView);
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        try {
            if (!view->isBlockingNotifications())
                call(view.data());
        } catch (const Exception &e) {
            e.showException(tr("Exception thrown by view %1.").arg(view->widgetInfo().tabName));
        }
    }

    if (nodeInstanceView() && !nodeInstanceView()->isBlockingNotifications())
        call(nodeInstanceView());

    if (resetModel)
        resetModelByRewriter(description);
}

template<typename Callable>
void ModelPrivate::notifyNormalViewsLast(Callable call)
{
    bool resetModel = false;
    QString description;

    try {
        if (m_rewriterView && !m_rewriterView->isBlockingNotifications())
            call(m_rewriterView);
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView() && !nodeInstanceView()->isBlockingNotifications())
        call(nodeInstanceView());

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        if (!view->isBlockingNotifications())
            call(view.data());
    }

    if (resetModel)
        resetModelByRewriter(description);
}

template<typename Callable>
void ModelPrivate::notifyInstanceChanges(Callable call)
{
    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        if (!view->isBlockingNotifications())
            call(view.data());
    }
}

void ModelPrivate::notifyAuxiliaryDataChanged(const InternalNodePointer &node,
                                              AuxiliaryDataKeyView key,
                                              const QVariant &data)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        ModelNode modelNode(node, m_model, view);
        view->auxiliaryDataChanged(modelNode, key, data);
    });
}

void ModelPrivate::notifyNodeSourceChanged(const InternalNodePointer &node,
                                           const QString &newNodeSource)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        ModelNode ModelNode(node, m_model, view);
        view->nodeSourceChanged(ModelNode, newNodeSource);
    });
}

void ModelPrivate::notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    notifyNodeInstanceViewLast(
        [&](AbstractView *view) { view->rootNodeTypeChanged(type, majorVersion, minorVersion); });
}

void ModelPrivate::notifyInstancePropertyChange(const QList<QPair<ModelNode, PropertyName>> &propertyPairList)
{
    notifyInstanceChanges([&](AbstractView *view) {
        using ModelNodePropertyPair = QPair<ModelNode, PropertyName>;
        QList<QPair<ModelNode, PropertyName>> adaptedPropertyList;
        for (const ModelNodePropertyPair &propertyPair : propertyPairList) {
            ModelNodePropertyPair newPair(ModelNode{propertyPair.first.internalNode(), m_model, view}, propertyPair.second);
            adaptedPropertyList.append(newPair);
        }
        view->instancePropertyChanged(adaptedPropertyList);
    });
}

void ModelPrivate::notifyInstanceErrorChange(const QVector<qint32> &instanceIds)
{
    notifyInstanceChanges([&](AbstractView *view) {
        QVector<ModelNode> errorNodeList;
        errorNodeList.reserve(instanceIds.size());
        for (qint32 instanceId : instanceIds)
            errorNodeList.emplace_back(m_model->d->nodeForInternalId(instanceId), m_model, view);
        view->instanceErrorChanged(errorNodeList);
    });
}

void ModelPrivate::notifyInstancesCompleted(const QVector<ModelNode> &modelNodeVector)
{
    auto internalNodes = toInternalNodeList(modelNodeVector);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesCompleted(toModelNodeList(internalNodes, view));
    });
}

namespace {
QMultiHash<ModelNode, InformationName> convertModelNodeInformationHash(
    const QMultiHash<ModelNode, InformationName> &informationChangeHash, AbstractView *view)
{
    QMultiHash<ModelNode, InformationName> convertedModelNodeInformationHash;

    for (auto it = informationChangeHash.cbegin(), end = informationChangeHash.cend(); it != end; ++it)
        convertedModelNodeInformationHash.insert(ModelNode(it.key(), view), it.value());

    return convertedModelNodeInformationHash;
}
} // namespace

void ModelPrivate::notifyInstancesInformationsChange(
    const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    notifyInstanceChanges([&](AbstractView *view) {
        view->instanceInformationsChanged(convertModelNodeInformationHash(informationChangeHash, view));
    });
}

void ModelPrivate::notifyInstancesRenderImageChanged(const QVector<ModelNode> &modelNodeVector)
{
    auto internalNodes = toInternalNodeList(modelNodeVector);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesRenderImageChanged(toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyInstancesPreviewImageChanged(const QVector<ModelNode> &modelNodeVector)
{
    auto internalNodes = toInternalNodeList(modelNodeVector);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesPreviewImageChanged(toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyInstancesChildrenChanged(const QVector<ModelNode> &modelNodeVector)
{
    auto internalNodes = toInternalNodeList(modelNodeVector);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesChildrenChanged(toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyCurrentStateChanged(const ModelNode &node)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->currentStateChanged(ModelNode(node.internalNode(), m_model, view));
    });
}

void ModelPrivate::notifyCurrentTimelineChanged(const ModelNode &node)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->currentTimelineChanged(ModelNode(node.internalNode(), m_model, view));
    });
}

void ModelPrivate::notifyRenderImage3DChanged(const QImage &image)
{
    notifyInstanceChanges([&](AbstractView *view) { view->renderImage3DChanged(image); });
}

void ModelPrivate::notifyUpdateActiveScene3D(const QVariantMap &sceneState)
{
    notifyInstanceChanges([&](AbstractView *view) { view->updateActiveScene3D(sceneState); });
}

void ModelPrivate::notifyModelNodePreviewPixmapChanged(const ModelNode &node,
                                                       const QPixmap &pixmap,
                                                       const QByteArray &requestId)
{
    notifyInstanceChanges(
        [&](AbstractView *view) { view->modelNodePreviewPixmapChanged(node, pixmap, requestId); });
}

void ModelPrivate::notifyImport3DSupportChanged(const QVariantMap &supportMap)
{
    notifyInstanceChanges([&](AbstractView *view) { view->updateImport3DSupport(supportMap); });
}

void ModelPrivate::notifyNodeAtPosResult(const ModelNode &modelNode, const QVector3D &pos3d)
{
    notifyInstanceChanges([&](AbstractView *view) { view->nodeAtPosReady(modelNode, pos3d); });
}

void ModelPrivate::notifyView3DAction(View3DActionType type, const QVariant &value)
{
    notifyNormalViewsLast([&](AbstractView *view) { view->view3DAction(type, value); });
}

void ModelPrivate::notifyDragStarted(QMimeData *mimeData)
{
    notifyInstanceChanges([&](AbstractView *view) { view->dragStarted(mimeData); });
}

void ModelPrivate::notifyDragEnded()
{
    notifyInstanceChanges([&](AbstractView *view) { view->dragEnded(); });
}

void ModelPrivate::notifyRewriterBeginTransaction()
{
    notifyNodeInstanceViewLast([&](AbstractView *view) { view->rewriterBeginTransaction(); });
}

void ModelPrivate::notifyRewriterEndTransaction()
{
    notifyNodeInstanceViewLast([&](AbstractView *view) { view->rewriterEndTransaction(); });
}

void ModelPrivate::notifyInstanceToken(const QString &token, int number,
                                       const QVector<ModelNode> &modelNodeVector)
{
    auto internalNodes = toInternalNodeList(modelNodeVector);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesToken(token, number, toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyCustomNotification(const AbstractView *senderView,
                                            const QString &identifier,
                                            const QList<ModelNode> &modelNodeList,
                                            const QList<QVariant> &data)
{
    auto internalList = toInternalNodeList(modelNodeList);
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->customNotification(senderView, identifier, toModelNodeList(internalList, view), data);
    });
}

void ModelPrivate::notifyCustomNotificationTo(AbstractView *view,
                                              const CustomNotificationPackage &package)
{
    if (view)
        view->customNotification(package);
}

void ModelPrivate::notifyPropertiesRemoved(const QList<PropertyPair> &propertyPairList)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        QList<AbstractProperty> propertyList;
        propertyList.reserve(propertyPairList.size());
        for (const PropertyPair &propertyPair : propertyPairList)
            propertyList.emplace_back(propertyPair.second, propertyPair.first, m_model, view);

        view->propertiesRemoved(propertyList);
    });
}

void ModelPrivate::notifyPropertiesAboutToBeRemoved(const QList<InternalProperty *> &internalPropertyList)
{
    bool resetModel = false;
    QString description;

    try {
        if (m_rewriterView) {
            QList<AbstractProperty> propertyList;
            for (InternalProperty *property : internalPropertyList) {
                AbstractProperty newProperty(property->name(),
                                             property->propertyOwner(),
                                             m_model,
                                             m_rewriterView);
                propertyList.append(newProperty);
            }

            m_rewriterView->propertiesAboutToBeRemoved(propertyList);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        QList<AbstractProperty> propertyList;
        Q_ASSERT(view != nullptr);
        for (auto property : internalPropertyList) {
            AbstractProperty newProperty(property->name(), property->propertyOwner(), m_model, view.data());
            propertyList.append(newProperty);
        }

        try {
            view->propertiesAboutToBeRemoved(propertyList);
        } catch (const RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }

    if (nodeInstanceView()) {
        QList<AbstractProperty> propertyList;
        for (auto property : internalPropertyList) {
            AbstractProperty newProperty(property->name(), property->propertyOwner(), m_model, nodeInstanceView());
            propertyList.append(newProperty);
        }

        nodeInstanceView()->propertiesAboutToBeRemoved(propertyList);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::setAuxiliaryData(const InternalNodePointer &node,
                                    const AuxiliaryDataKeyView &key,
                                    const QVariant &data)
{
    bool changed = false;

    if (data.isValid())
        changed = node->setAuxiliaryData(key, data);
    else
        changed = node->removeAuxiliaryData(key);

    if (changed)
        notifyAuxiliaryDataChanged(node, key, data);
}

void ModelPrivate::resetModelByRewriter(const QString &description)
{
    if (m_rewriterView) {
        m_rewriterView->resetToLastCorrectQml();

        throw RewritingException(__LINE__,
                                 __FUNCTION__,
                                 __FILE__,
                                 description.toUtf8(),
                                 rewriterView()->textModifierContent());
    }
}

void ModelPrivate::attachView(AbstractView *view)
{
    if (!view)
        return;

    if (!view->isEnabled())
        return;

    if (view->isAttached()) {
        if (view->model() == m_model)
            return;
        else
            view->model()->detachView(view);
    }

    m_viewList.append(view);

    view->modelAttached(m_model);
}

void ModelPrivate::detachView(AbstractView *view, bool notifyView)
{
    if (!view->isAttached())
        return;

    if (notifyView)
        view->modelAboutToBeDetached(m_model);
    m_viewList.removeOne(view);
}

void ModelPrivate::notifyNodeCreated(const InternalNodePointer &newInternalNodePointer)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        view->nodeCreated(ModelNode{newInternalNodePointer, m_model, view});
    });
}

void ModelPrivate::notifyNodeAboutToBeRemoved(const InternalNodePointer &internalNodePointer)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeAboutToBeRemoved(ModelNode{internalNodePointer, m_model, view});
    });
}

void ModelPrivate::notifyNodeRemoved(const InternalNodePointer &removedNode,
                                     const InternalNodePointer &parentNode,
                                     PropertyNameView parentPropertyName,
                                     AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        view->nodeRemoved(ModelNode{removedNode, m_model, view},
                          NodeAbstractProperty{parentPropertyName, parentNode, m_model, view},
                          propertyChange);
    });
}

void ModelPrivate::notifyNodeTypeChanged(const InternalNodePointer &node,
                                         const TypeName &type,
                                         int majorVersion,
                                         int minorVersion)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeTypeChanged(ModelNode{node, m_model, view},
                              type,
                              majorVersion,
                              minorVersion);
    });
}

void ModelPrivate::notifyNodeIdChanged(const InternalNodePointer &node,
                                       const QString &newId,
                                       const QString &oldId)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeIdChanged(ModelNode{node, m_model, view}, newId, oldId);
    });
}

void ModelPrivate::notifyBindingPropertiesAboutToBeChanged(
    const QList<InternalBindingProperty *> &internalPropertyList)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QList<BindingProperty> propertyList;
        propertyList.reserve(internalPropertyList.size());
        for (auto bindingProperty : internalPropertyList) {
            propertyList.emplace_back(bindingProperty->name(),
                                      bindingProperty->propertyOwner(),
                                      m_model,
                                      view);
        }
        view->bindingPropertiesAboutToBeChanged(propertyList);
    });
}

void ModelPrivate::notifyBindingPropertiesChanged(const QList<InternalBindingProperty *> &internalPropertyList,
                                                  AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QList<BindingProperty> propertyList;
        propertyList.reserve(internalPropertyList.size());
        for (auto bindingProperty : internalPropertyList) {
            propertyList.emplace_back(bindingProperty->name(),
                                      bindingProperty->propertyOwner(),
                                      m_model,
                                      view);
        }
        view->bindingPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifySignalHandlerPropertiesChanged(
    const QVector<InternalSignalHandlerProperty *> &internalPropertyList,
    AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QVector<SignalHandlerProperty> propertyList;
        propertyList.reserve(internalPropertyList.size());
        for (auto signalHandlerProperty : internalPropertyList) {
            propertyList.emplace_back(signalHandlerProperty->name(),
                                      signalHandlerProperty->propertyOwner(),
                                      m_model,
                                      view);
        }
        view->signalHandlerPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifySignalDeclarationPropertiesChanged(
    const QVector<InternalSignalDeclarationProperty *> &internalPropertyList,
    AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QVector<SignalDeclarationProperty> propertyList;
        propertyList.reserve(internalPropertyList.size());
        for (auto signalHandlerProperty : internalPropertyList) {
            propertyList.emplace_back(signalHandlerProperty->name(),
                                      signalHandlerProperty->propertyOwner(),
                                      m_model,
                                      view);
        }
        view->signalDeclarationPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifyScriptFunctionsChanged(const InternalNodePointer &node,
                                                const QStringList &scriptFunctionList)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        view->scriptFunctionsChanged(ModelNode{node, m_model, view}, scriptFunctionList);
    });
}

void ModelPrivate::notifyVariantPropertiesChanged(const InternalNodePointer &node,
                                                  const PropertyNameViews &propertyNameViews,
                                                  AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QList<VariantProperty> propertyList;
        propertyList.reserve(propertyNameViews.size());
        for (PropertyNameView propertyName : propertyNameViews)
            propertyList.emplace_back(propertyName, node, m_model, view);

        view->variantPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifyNodeAboutToBeReparent(const InternalNodePointer &node,
                                               const InternalNodePointer &newParent,
                                               PropertyNameView newPropertyName,
                                               const InternalNodePointer &oldParent,
                                               PropertyNameView oldPropertyName,
                                               AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent && oldParent->isValid)
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, m_model, view);

        if (!newPropertyName.isEmpty() && newParent && newParent->isValid) {
            newProperty = NodeAbstractProperty(newPropertyName,
                                               newParent,
                                               m_model,
                                               view);
        }

        ModelNode modelNode(node, m_model, view);
        view->nodeAboutToBeReparented(modelNode, newProperty, oldProperty, propertyChange);
    });
}

void ModelPrivate::notifyNodeReparent(const InternalNodePointer &node,
                                      const InternalNodeAbstractProperty *newPropertyParent,
                                      const InternalNodePointer &oldParent,
                                      PropertyNameView oldPropertyName,
                                      AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent->isValid)
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, m_model, view);

        if (newPropertyParent) {
            newProperty = NodeAbstractProperty(newPropertyParent->name(),
                                               newPropertyParent->propertyOwner(),
                                               m_model,
                                               view);
        }

        ModelNode modelNode(node, m_model, view);

        view->nodeReparented(modelNode, newProperty, oldProperty, propertyChange);
    });
}

void ModelPrivate::notifyNodeOrderChanged(const InternalNodeListProperty *internalListProperty,
                                          const InternalNodePointer &node,
                                          int oldIndex)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeListProperty nodeListProperty(internalListProperty->name(),
                                          internalListProperty->propertyOwner(),
                                          m_model,
                                          view);
        view->nodeOrderChanged(nodeListProperty, ModelNode(node, m_model, view), oldIndex);
    });
}

void ModelPrivate::notifyNodeOrderChanged(const InternalNodeListProperty *internalListProperty)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeListProperty nodeListProperty(internalListProperty->name(),
                                          internalListProperty->propertyOwner(),
                                          m_model,
                                          view);
        view->nodeOrderChanged(nodeListProperty);
    });
}

void ModelPrivate::setSelectedNodes(const FewNodes &selectedNodeList)
{
    auto sortedSelectedList = Utils::filtered(selectedNodeList, [](const auto &node) {
        return node && node->isValid;
    });

    Utils::sort(sortedSelectedList);
    sortedSelectedList.erase(std::unique(sortedSelectedList.begin(), sortedSelectedList.end()),
                             sortedSelectedList.end());

    if (sortedSelectedList == m_selectedInternalNodes)
        return;

    auto flowToken = traceToken.tickWithFlow("selected model nodes");

    if constexpr (decltype(traceToken)::categoryIsActive()) { // the compiler should optimize it away but to be sure
        std::set_difference(sortedSelectedList.begin(),
                            sortedSelectedList.end(),
                            m_selectedInternalNodes.begin(),
                            m_selectedInternalNodes.end(),
                            Utils::make_iterator([&](const auto &node) {
                                node->traceToken.tick(flowToken, "select model node");
                            }));
    }

    const auto lastSelectedNodeList = std::move(m_selectedInternalNodes);
    m_selectedInternalNodes = sortedSelectedList;

    if constexpr (decltype(traceToken)::categoryIsActive()) { // the compiler should optimize it away but to be sure
        std::set_difference(lastSelectedNodeList.begin(),
                            lastSelectedNodeList.end(),
                            m_selectedInternalNodes.begin(),
                            m_selectedInternalNodes.end(),
                            Utils::make_iterator([&](const auto &node) {
                                node->traceToken.tick(flowToken, "deselect model node");
                            }));
    }

    changeSelectedNodes(sortedSelectedList, lastSelectedNodeList);
}

void ModelPrivate::clearSelectedNodes()
{
    auto tracer = traceToken.begin("clear selected model nodes");

    auto lastSelectedNodeList = m_selectedInternalNodes;
    m_selectedInternalNodes.clear();
    changeSelectedNodes(m_selectedInternalNodes, lastSelectedNodeList);
}

void ModelPrivate::removeAuxiliaryData(const InternalNodePointer &node, const AuxiliaryDataKeyView &key)
{
    bool removed = node->removeAuxiliaryData(key);

    if (removed)
        notifyAuxiliaryDataChanged(node, key, QVariant());
}

QList<ModelNode> ModelPrivate::toModelNodeList(Utils::span<const InternalNodePointer> nodeList,
                                               AbstractView *view) const
{
    QList<ModelNode> modelNodeList;
    modelNodeList.reserve(nodeList.size());
    for (const InternalNodePointer &node : nodeList)
        modelNodeList.emplace_back(node, m_model, view);

    return modelNodeList;
}

ModelPrivate::ManyNodes ModelPrivate::toInternalNodeList(const QList<ModelNode> &modelNodeList) const
{
    ManyNodes newNodeList;
    newNodeList.reserve(modelNodeList.size());
    for (const ModelNode &modelNode : modelNodeList)
        newNodeList.append(modelNode.internalNode());

    return newNodeList;
}

QList<InternalProperty *> ModelPrivate::toInternalProperties(const AbstractProperties &properties)
{
    QList<InternalProperty *> internalProperties;
    internalProperties.reserve(properties.size());

    for (const auto &property : properties) {
        if (property.m_internalNode) {
            if (auto internalProperty = property.m_internalNode->property(property.m_propertyName))
                internalProperties.push_back(internalProperty);
        }
    }

    return internalProperties;
}

QList<std::tuple<InternalBindingProperty *, QString>> ModelPrivate::toInternalBindingProperties(
    const ModelResourceSet::SetExpressions &setExpressions)
{
    QList<std::tuple<InternalBindingProperty *, QString>> internalProperties;
    internalProperties.reserve(setExpressions.size());

    for (const auto &setExpression : setExpressions) {
        const auto &property = setExpression.property;
        if (property.m_internalNode) {
            if (auto internalProperty = property.m_internalNode->bindingProperty(
                    property.m_propertyName))
                internalProperties.emplace_back(internalProperty, setExpression.expression);
        }
    }

    return internalProperties;
}

void ModelPrivate::changeSelectedNodes(const FewNodes &newSelectedNodeList,
                                       const FewNodes &oldSelectedNodeList)
{
    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        Q_ASSERT(view != nullptr);
        view->selectedNodesChanged(toModelNodeList(newSelectedNodeList, view.data()),
                                   toModelNodeList(oldSelectedNodeList, view.data()));
    }

    if (nodeInstanceView()) {
        nodeInstanceView()->selectedNodesChanged(toModelNodeList(newSelectedNodeList, nodeInstanceView()),
                                                 toModelNodeList(oldSelectedNodeList, nodeInstanceView()));
    }
}

const ModelPrivate::FewNodes &ModelPrivate::selectedNodes() const
{
    static FewNodes empty;
    for (const InternalNodePointer &node : m_selectedInternalNodes) {
        if (!node->isValid)
            return empty;
    }

    return m_selectedInternalNodes;
}

void ModelPrivate::selectNode(const InternalNodePointer &node)
{
    if (selectedNodes().contains(node))
        return;

    FewNodes selectedNodeList(selectedNodes());
    selectedNodeList += node;
    setSelectedNodes(selectedNodeList);
}

void ModelPrivate::deselectNode(const InternalNodePointer &node)
{
    FewNodes selectedNodeList(selectedNodes());
    bool isRemoved = selectedNodeList.removeOne(node);

    if (isRemoved)
        setSelectedNodes(selectedNodeList);
}

void ModelPrivate::removePropertyWithoutNotification(InternalProperty *property)
{
    if (auto nodeListProperty = property->to<PropertyType::NodeList>()) {
        const auto allSubNodes = nodeListProperty->allSubNodes();
        for (const InternalNodePointer &node : allSubNodes)
            removeNodeFromModel(node);
    } else if (auto nodeProperty = property->to<PropertyType::Node>()) {
        if (auto node = nodeProperty->node())
            removeNodeFromModel(node);
    }

    auto propertyOwner = property->propertyOwner();
    propertyOwner->removeProperty(property->name());
}

static QList<PropertyPair> toPropertyPairList(const QList<InternalProperty *> &propertyList)
{
    QList<PropertyPair> propertyPairList;
    propertyPairList.reserve(propertyList.size());

    for (const InternalProperty *property : propertyList)
        propertyPairList.emplace_back(property->propertyOwner(), property->name());

    return propertyPairList;
}

void ModelPrivate::removePropertyAndRelatedResources(InternalProperty *property)
{
    if (m_resourceManagement) {
        handleResourceSet(m_resourceManagement->removeProperties(
            {AbstractProperty{property->name(), property->propertyOwner(), m_model, nullptr}},
            m_model));
    } else {
        removeProperty(property);
    }
}

void ModelPrivate::removeProperty(InternalProperty *property)
{
    removeProperties({property});
}

void ModelPrivate::removeProperties(const QList<InternalProperty *> &properties)
{
    if (properties.isEmpty())
        return;

    notifyPropertiesAboutToBeRemoved(properties);

    const QList<PropertyPair> propertyPairList = toPropertyPairList(properties);

    for (auto property : properties)
        removePropertyWithoutNotification(property);

    notifyPropertiesRemoved(propertyPairList);
}

void ModelPrivate::setBindingProperty(const InternalNodePointer &node,
                                      PropertyNameView name,
                                      const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    InternalBindingProperty *bindingProperty = nullptr;
    if (auto property = node->property(name)) {
        bindingProperty = property->to<PropertyType::Binding>();
    } else {
        bindingProperty = node->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    notifyBindingPropertiesAboutToBeChanged({bindingProperty});
    bindingProperty->setExpression(expression);
    notifyBindingPropertiesChanged({bindingProperty}, propertyChange);
}

void ModelPrivate::setBindingProperties(const ModelResourceSet::SetExpressions &setExpressions)
{
    if (setExpressions.isEmpty())
        return;

    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;

    auto bindingPropertiesWithExpressions = toInternalBindingProperties(setExpressions);

    auto bindingProperties = CoreUtils::to<QList>(bindingPropertiesWithExpressions | std::views::keys);

    notifyBindingPropertiesAboutToBeChanged(bindingProperties);
    for (const auto &[bindingProperty, expression] : bindingPropertiesWithExpressions)
        bindingProperty->setExpression(expression);
    notifyBindingPropertiesChanged(bindingProperties, propertyChange);
}

void ModelPrivate::setSignalHandlerProperty(const InternalNodePointer &node,
                                            PropertyNameView name,
                                            const QString &source)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    InternalSignalHandlerProperty *signalHandlerProperty = nullptr;
    if (auto property = node->property(name)) {
        signalHandlerProperty = property->to<PropertyType::SignalHandler>();
    } else {
        signalHandlerProperty = node->addSignalHandlerProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    signalHandlerProperty->setSource(source);
    notifySignalHandlerPropertiesChanged({signalHandlerProperty}, propertyChange);
}

void ModelPrivate::setSignalDeclarationProperty(const InternalNodePointer &node,
                                                PropertyNameView name,
                                                const QString &signature)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    InternalSignalDeclarationProperty *signalDeclarationProperty = nullptr;
    if (auto property = node->property(name)) {
        signalDeclarationProperty = property->to<PropertyType::SignalDeclaration>();
    } else {
        signalDeclarationProperty = node->addSignalDeclarationProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    signalDeclarationProperty->setSignature(signature);
    notifySignalDeclarationPropertiesChanged({signalDeclarationProperty}, propertyChange);
}

void ModelPrivate::setVariantProperty(const InternalNodePointer &node,
                                      PropertyNameView name,
                                      const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    InternalVariantProperty *variantProperty = nullptr;
    if (auto property = node->property(name)) {
        variantProperty = property->to<PropertyType::Variant>();
    } else {
        variantProperty = node->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    variantProperty->setValue(value);
    variantProperty->resetDynamicTypeName();
    notifyVariantPropertiesChanged(node, PropertyNameViews({name}), propertyChange);
}

void ModelPrivate::setDynamicVariantProperty(const InternalNodePointer &node,
                                             PropertyNameView name,
                                             const TypeName &dynamicPropertyType,
                                             const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    InternalVariantProperty *variantProperty = nullptr;
    if (auto property = node->property(name)) {
        variantProperty = property->to<PropertyType::Variant>();
    } else {
        variantProperty = node->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    variantProperty->setDynamicValue(dynamicPropertyType, value);
    notifyVariantPropertiesChanged(node, PropertyNameViews({name}), propertyChange);
}

void ModelPrivate::setDynamicBindingProperty(const InternalNodePointer &node,
                                             PropertyNameView name,
                                             const TypeName &dynamicPropertyType,
                                             const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    InternalBindingProperty *bindingProperty = nullptr;
    if (auto property = node->property(name)) {
        bindingProperty = property->to<PropertyType::Binding>();
    } else {
        bindingProperty = node->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    notifyBindingPropertiesAboutToBeChanged({bindingProperty});
    bindingProperty->setDynamicExpression(dynamicPropertyType, expression);
    notifyBindingPropertiesChanged({bindingProperty}, propertyChange);
}

void ModelPrivate::reparentNode(const InternalNodePointer &parentNode,
                                PropertyNameView name,
                                const InternalNodePointer &childNode,
                                bool list,
                                const TypeName &dynamicTypeName)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;

    InternalNodeAbstractPropertyPointer oldParentProperty(childNode->parentProperty());
    InternalNodePointer oldParentNode;
    Utils::SmallString oldParentPropertyName;
    if (oldParentProperty && oldParentProperty->isValid()) {
        oldParentNode = childNode->parentProperty()->propertyOwner();
        oldParentPropertyName = childNode->parentProperty()->name();
    }

    notifyNodeAboutToBeReparent(childNode,
                                parentNode,
                                name,
                                oldParentNode,
                                oldParentPropertyName,
                                propertyChange);

    InternalNodeAbstractProperty *newParentProperty = nullptr;
    if (auto property = parentNode->property(name)) {
        newParentProperty = property->to<PropertyType::Node, PropertyType::NodeList>();
    } else {
        if (list)
            newParentProperty = parentNode->addNodeListProperty(name);
        else
            newParentProperty = parentNode->addNodeProperty(name, dynamicTypeName);

        propertyChange |= AbstractView::PropertiesAdded;
    }

    Q_ASSERT(newParentProperty);

    if (newParentProperty) {
        childNode->setParentProperty(
            newParentProperty->toShared<PropertyType::Node, PropertyType::NodeList>());
    }

    if (oldParentProperty && oldParentProperty->isValid() && oldParentProperty->isEmpty()) {
        removePropertyWithoutNotification(oldParentProperty.get());

        propertyChange |= AbstractView::EmptyPropertiesRemoved;
    }

    notifyNodeReparent(childNode, newParentProperty, oldParentNode, oldParentPropertyName, propertyChange);
}

void ModelPrivate::clearParent(const InternalNodePointer &node)
{
    InternalNodeAbstractPropertyPointer oldParentProperty(node->parentProperty());
    InternalNodePointer oldParentNode;
    Utils::SmallString oldParentPropertyName;
    if (oldParentProperty->isValid()) {
        oldParentNode = node->parentProperty()->propertyOwner();
        oldParentPropertyName = node->parentProperty()->name();
    }

    node->resetParentProperty();
    notifyNodeReparent(node,
                       nullptr,
                       oldParentNode,
                       oldParentPropertyName,
                       AbstractView::NoAdditionalChanges);
}

void ModelPrivate::changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion)
{
    Q_ASSERT(rootNode());

    m_rootInternalNode->traceToken.tick("type name", keyValue("type name", type));

    m_rootInternalNode->typeName = type;
    m_rootInternalNode->majorVersion = majorVersion;
    m_rootInternalNode->minorVersion = minorVersion;
    setTypeId(m_rootInternalNode.get(), type);
    notifyRootNodeTypeChanged(QString::fromUtf8(type), majorVersion, minorVersion);
}

void ModelPrivate::setScriptFunctions(const InternalNodePointer &node, const QStringList &scriptFunctionList)
{
    m_rootInternalNode->traceToken.tick("script function");

    node->scriptFunctions = scriptFunctionList;

    notifyScriptFunctionsChanged(node, scriptFunctionList);
}

void ModelPrivate::setNodeSource(const InternalNodePointer &node, const QString &nodeSource)
{
    m_rootInternalNode->traceToken.tick("node source");

    node->nodeSource = nodeSource;
    notifyNodeSourceChanged(node, nodeSource);
}

void ModelPrivate::changeNodeOrder(const InternalNodePointer &parentNode,
                                   PropertyNameView listPropertyName,
                                   int from,
                                   int to)
{
    auto nodeList = parentNode->nodeListProperty(listPropertyName);
    Q_ASSERT(nodeList);
    nodeList->slide(from, to);

    const InternalNodePointer internalNode = nodeList->nodeList().at(to);
    notifyNodeOrderChanged(nodeList, internalNode, from);
}

void ModelPrivate::setRewriterView(RewriterView *rewriterView)
{
    if (rewriterView == m_rewriterView.data())
        return;

    Q_ASSERT(!(rewriterView && m_rewriterView));

    if (rewriterView && rewriterView->kind() != AbstractView::Kind::Rewriter)
        return;

    if (m_rewriterView)
        m_rewriterView->modelAboutToBeDetached(m_model);

    m_rewriterView = rewriterView;

    if (rewriterView)
        rewriterView->modelAttached(m_model);
}

RewriterView *ModelPrivate::rewriterView() const
{
    return m_rewriterView.data();
}

void ModelPrivate::setNodeInstanceView(AbstractView *nodeInstanceView)
{
    if (nodeInstanceView == m_nodeInstanceView)
        return;

    if (nodeInstanceView && nodeInstanceView->kind() != AbstractView::Kind::NodeInstance)
        return;

    if (nodeInstanceView && nodeInstanceView->isAttached())
        nodeInstanceView->model()->setNodeInstanceView(nullptr);

    if (m_nodeInstanceView)
        m_nodeInstanceView->modelAboutToBeDetached(m_model);

    m_nodeInstanceView = nodeInstanceView;

    if (nodeInstanceView)
        nodeInstanceView->modelAttached(m_model);
}

AbstractView *ModelPrivate::nodeInstanceView() const
{
    return m_nodeInstanceView.data();
}

InternalNodePointer ModelPrivate::currentTimelineNode() const
{
    return m_currentTimelineNode;
}

InternalNodePointer ModelPrivate::nodeForId(const QString &id) const
{
    return m_idNodeHash.value(id);
}

bool ModelPrivate::hasId(const QString &id) const
{
    return m_idNodeHash.contains(id);
}

InternalNodePointer ModelPrivate::nodeForInternalId(qint32 internalId) const
{
    return m_internalIdNodeHash.value(internalId);
}

bool ModelPrivate::hasNodeForInternalId(qint32 internalId) const
{
    return m_internalIdNodeHash.contains(internalId);
}

ModelPrivate::ManyNodes ModelPrivate::allNodesOrdered() const
{
    if (!m_rootInternalNode || !m_rootInternalNode->isValid)
        return {};

    // the nodes must be ordered.

    ManyNodes nodeList;
    nodeList.append(m_rootInternalNode);
    m_rootInternalNode->addSubNodes(nodeList);
    // FIXME: This is horribly expensive compared to a loop.

    auto nodesSorted = nodeList;
    std::ranges::sort(nodesSorted);
    std::ranges::set_difference(m_nodes, nodesSorted, std::back_inserter(nodeList));

    return nodeList;
}

ModelPrivate::ManyNodes ModelPrivate::allNodesUnordered() const
{
    return m_nodes;
}

bool ModelPrivate::isWriteLocked() const
{
    return m_writeLock;
}

InternalNodePointer ModelPrivate::currentStateNode() const
{
    return m_currentStateNode;
}

WriteLocker::WriteLocker(ModelPrivate *model)
    : m_model(model)
{
    Q_ASSERT(model);
    if (m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
    QTC_CHECK(!m_model->m_writeLock);
    Q_ASSERT(!m_model->m_writeLock);
    model->m_writeLock = true;
}

WriteLocker::WriteLocker(Model *model)
    : m_model(model->d.get())
{
    Q_ASSERT(model->d.get());
    if (m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
    QTC_CHECK(!m_model->m_writeLock);
    Q_ASSERT(!m_model->m_writeLock);
    m_model->m_writeLock = true;
}

WriteLocker::~WriteLocker()
{
    if (!m_model->m_writeLock)
        qWarning() << "QmlDesigner: WriterLocker out of sync!!!";
    // FIXME: Enable it again
    QTC_CHECK(m_model->m_writeLock);
    Q_ASSERT(m_model->m_writeLock);
    m_model->m_writeLock = false;
}

void WriteLocker::unlock(Model *model)
{
    model->d->m_writeLock = false;
}

void WriteLocker::lock(Model *model)
{
    model->d->m_writeLock = true;
}

} // namespace Internal

Model::Model(ProjectStorageDependencies projectStorageDependencies,
             const TypeName &typeName,
             int major,
             int minor,
             Model *metaInfoProxyModel,
             std::unique_ptr<ModelResourceManagementInterface> resourceManagement)
    : d(std::make_unique<Internal::ModelPrivate>(this,
                                                 projectStorageDependencies,
                                                 typeName,
                                                 major,
                                                 minor,
                                                 metaInfoProxyModel,
                                                 std::move(resourceManagement)))
{}

Model::Model(ProjectStorageDependencies projectStorageDependencies,
             Utils::SmallStringView typeName,
             Imports imports,
             const QUrl &fileUrl,
             std::unique_ptr<ModelResourceManagementInterface> resourceManagement)
    : d(std::make_unique<Internal::ModelPrivate>(this,
                                                 projectStorageDependencies,
                                                 typeName,
                                                 std::move(imports),
                                                 fileUrl,
                                                 std::move(resourceManagement)))
{}

#ifndef QDS_USE_PROJECTSTORAGE
Model::Model(const TypeName &typeName,
             int major,
             int minor,
             Model *metaInfoProxyModel,
             std::unique_ptr<ModelResourceManagementInterface> resourceManagement)
    : d(std::make_unique<Internal::ModelPrivate>(
        this, typeName, major, minor, metaInfoProxyModel, std::move(resourceManagement)))
{}
#endif

ModelPointer Model::createModel(const TypeName &typeName,
                                std::unique_ptr<ModelResourceManagementInterface> resourceManagement)
{
    return Model::create({*d->projectStorage, *d->pathCache},
                         typeName,
                         imports(),
                         fileUrl(),
                         std::move(resourceManagement));
}

Model::~Model() = default;

const Imports &Model::imports() const
{
    return d->imports();
}

ModuleIds Model::moduleIds() const
{
    return {};
}

Storage::Info::ExportedTypeName Model::exportedTypeNameForMetaInfo(const NodeMetaInfo &) const
{
    return {};
}

Imports Model::possibleImports() const
{
#ifdef QDS_USE_PROJECTSTORAGE
    return {};
#else
    return d->m_possibleImportList;
#endif
}

Imports Model::usedImports() const
{
#ifdef QDS_USE_PROJECTSTORAGE
    return {};
#else
    return d->m_usedImportList;
#endif
}

void Model::changeImports(Imports importsToBeAdded, Imports importsToBeRemoved)
{
    d->changeImports(std::move(importsToBeAdded), std::move(importsToBeRemoved));
}

#ifndef QDS_USE_PROJECTSTORAGE
void Model::setPossibleImports(Imports possibleImports)
{
    auto tracer = d->traceToken.begin("possible imports");

    std::sort(possibleImports.begin(), possibleImports.end());

    if (d->m_possibleImportList != possibleImports) {
        d->m_possibleImportList = std::move(possibleImports);
        d->notifyPossibleImportsChanged(d->m_possibleImportList);
    }
}
#endif

#ifndef QDS_USE_PROJECTSTORAGE
void Model::setUsedImports(Imports usedImports)
{
    auto tracer = d->traceToken.begin("used imports");

    std::sort(usedImports.begin(), usedImports.end());

    if (d->m_usedImportList != usedImports) {
        d->m_usedImportList = std::move(usedImports);
        d->notifyUsedImportsChanged(d->m_usedImportList);
    }
}
#endif

static bool compareVersions(const Import &import1, const Import &import2, bool allowHigherVersion)
{
    auto version1 = import1.toVersion();
    auto version2 = import2.toVersion();

    if (version2.isEmpty())
        return true;
    if (version1 == version2)
        return true;
    if (!allowHigherVersion)
        return false;

    return version1 >= version2;
}

bool Model::hasImport(const Import &import, bool ignoreAlias, bool allowHigherVersion) const
{
    if (imports().contains(import))
        return true;

    if (!ignoreAlias)
        return false;

    for (const Import &existingImport : imports()) {
        if (existingImport.isFileImport() && import.isFileImport()) {
            if (existingImport.file() == import.file())
                return true;
        }
        if (existingImport.isLibraryImport() && import.isLibraryImport()) {
            if (existingImport.url() == import.url()
                && compareVersions(existingImport, import, allowHigherVersion)) {
                return true;
            }
        }
    }

    return false;
}

bool Model::hasId(const QString &id) const
{
    return d->hasId(id);
}

bool Model::hasImport(const QString &importUrl) const
{
    return std::ranges::any_of(imports(), is_equal_to(importUrl), &Import::url);
}

QString Model::generateNewId(const QString &prefixName, const QString &fallbackPrefix) const
{
    return UniqueName::generateId(prefixName, fallbackPrefix, [&](const QString &id) {
        // Properties of the root node are not allowed for ids, because they are available in the
        // complete context without qualification.
        return hasId(id) || d->rootNode()->property(id.toUtf8());
    });
}

void Model::startDrag(std::unique_ptr<QMimeData> mimeData, const QPixmap &icon, QWidget *dragSource)
{
    d->notifyDragStarted(mimeData.get());

    d->drag = Utils::makeUniqueObjectPtr<QDrag>(dragSource);
    d->drag->setPixmap(icon);
    d->drag->setMimeData(mimeData.release());
    if (d->drag->exec() == Qt::IgnoreAction)
        endDrag();
}

void Model::endDrag()
{
    if (!d->drag)
        return;

    d->notifyDragEnded();
    d->drag.reset();
}

void Model::setCurrentStateNode(const ModelNode &node)
{
    Internal::WriteLocker locker(this);
    d->m_currentStateNode = node.internalNode();
    d->notifyCurrentStateChanged(node);
}

ModelNode Model::currentStateNode(AbstractView *view)
{
    return ModelNode(d->currentStateNode(), this, view);
}

void Model::setCurrentTimelineNode(const ModelNode &timeline)
{
    Internal::WriteLocker locker(this); // unsure about this locker

    d->m_currentTimelineNode = timeline.internalNode();

    d->notifyCurrentTimelineChanged(timeline);
}

NotNullPointer<const ProjectStorageType> Model::projectStorage() const
{
    return d->projectStorage;
}

const PathCacheType &Model::pathCache() const
{
    return *d->pathCache;
}

PathCacheType &Model::pathCache()
{
    return *d->pathCache;
}

void Model::emitInstancePropertyChange(AbstractView *view,
                                       const QList<QPair<ModelNode, PropertyName>> &propertyList)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancePropertyChange(propertyList);
}

void Model::emitInstanceErrorChange(AbstractView *view, const QVector<qint32> &instanceIds)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstanceErrorChange(instanceIds);
}

void Model::emitInstancesCompleted(AbstractView *view, const QVector<ModelNode> &nodeVector)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesCompleted(nodeVector);
}

void Model::emitInstanceInformationsChange(
    AbstractView *view, const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesInformationsChange(informationChangeHash);
}

void Model::emitInstancesRenderImageChanged(AbstractView *view, const QVector<ModelNode> &nodeVector)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesRenderImageChanged(nodeVector);
}

void Model::emitInstancesPreviewImageChanged(AbstractView *view, const QVector<ModelNode> &nodeVector)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesPreviewImageChanged(nodeVector);
}

void Model::emitInstancesChildrenChanged(AbstractView *view, const QVector<ModelNode> &nodeVector)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesChildrenChanged(nodeVector);
}

void Model::emitInstanceToken(AbstractView *view,
                              const QString &token,
                              int number,
                              const QVector<ModelNode> &nodeVector)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstanceToken(token, number, nodeVector);
}

void Model::emitRenderImage3DChanged(AbstractView *view, const QImage &image)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyRenderImage3DChanged(image);
}

void Model::emitUpdateActiveScene3D(AbstractView *view, const QVariantMap &sceneState)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyUpdateActiveScene3D(sceneState);
}

void Model::emitModelNodelPreviewPixmapChanged(AbstractView *view,
                                               const ModelNode &node,
                                               const QPixmap &pixmap,
                                               const QByteArray &requestId)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyModelNodePreviewPixmapChanged(node, pixmap, requestId);
}

void Model::emitImport3DSupportChanged(AbstractView *view, const QVariantMap &supportMap)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyImport3DSupportChanged(supportMap);
}

void Model::emitNodeAtPosResult(AbstractView *view, const ModelNode &modelNode, const QVector3D &pos3d)
{
    if (d->nodeInstanceView() == view) // never remove check
        d->notifyNodeAtPosResult(modelNode, pos3d);
}

void Model::emitView3DAction(View3DActionType type, const QVariant &value)
{
    d->notifyView3DAction(type, value);
}

void Model::emitDocumentMessage(const QString &error)
{
    emitDocumentMessage({DocumentMessage(error)});
}

void Model::emitDocumentMessage(const QList<DocumentMessage> &errors,
                                const QList<DocumentMessage> &warnings)
{
    d->setDocumentMessages(errors, warnings);
}

void Model::emitCustomNotification(AbstractView *view,
                                   const QString &identifier,
                                   const QList<ModelNode> &nodeList,
                                   const QList<QVariant> &data)
{
    d->notifyCustomNotification(view, identifier, nodeList, data);
}

void Model::sendCustomNotificationTo(AbstractView *to, const CustomNotificationPackage &package)
{
    d->notifyCustomNotificationTo(to, package);
}

void Model::sendCustomNotificationToNodeInstanceView(const CustomNotificationPackage &package)
{
    if (auto view = d->nodeInstanceView())
        d->notifyCustomNotificationTo(view, package);
}

void Model::emitRewriterBeginTransaction()
{
    d->notifyRewriterBeginTransaction();
}

void Model::emitRewriterEndTransaction()
{
    d->notifyRewriterEndTransaction();
}

void ModelDeleter::operator()(class Model *model)
{
    model->detachAllViews();
    delete model;
}

void Model::detachAllViews()
{
    d->detachAllViews();
}

bool Model::isImportPossible(const Import &import, bool ignoreAlias, bool allowHigherVersion) const
{
    if (imports().contains(import))
        return true;

    if (!ignoreAlias)
        return false;

    for (const Import &possibleImport : possibleImports()) {
        if (possibleImport.isFileImport() && import.isFileImport()) {
            if (possibleImport.file() == import.file())
                return true;
        }

        if (possibleImport.isLibraryImport() && import.isLibraryImport()) {
            if (possibleImport.url() == import.url()
                && compareVersions(possibleImport, import, allowHigherVersion)) {
                return true;
            }
        }
    }

    return false;
}

QStringList Model::importPaths() const
{
    if (rewriterView())
        return rewriterView()->importDirectories();

    QString documentDirectoryPath = QFileInfo(fileUrl().toLocalFile()).absolutePath();
    if (!documentDirectoryPath.isEmpty())
        return {documentDirectoryPath};

    return {};
}

Import Model::highestPossibleImport(const QString &importPath)
{
    Import candidate;

    for (const Import &import : possibleImports()) {
        if (import.url() == importPath) {
            if (candidate.isEmpty() || compareVersions(import, candidate, true))
                candidate = import;
        }
    }

    return candidate;
}

RewriterView *Model::rewriterView() const
{
    return d->rewriterView();
}

void Model::setRewriterView(RewriterView *rewriterView)
{
    d->setRewriterView(rewriterView);
}

const AbstractView *Model::nodeInstanceView() const
{
    return d->nodeInstanceView();
}

void Model::setNodeInstanceView(AbstractView *nodeInstanceView)
{
    d->setNodeInstanceView(nodeInstanceView);
}

/*!
 \brief Returns the model that is used for metainfo
 \return Returns itself if other metaInfoProxyModel does not exist
*/
Model *Model::metaInfoProxyModel() const
{
    if (d->m_metaInfoProxyModel)
        return d->m_metaInfoProxyModel->metaInfoProxyModel();

    return const_cast<Model *>(this);
}

void Model::setDocumentMessages(const QList<DocumentMessage> &errors,
                                const QList<DocumentMessage> &warnings)
{
    d->setDocumentMessages(errors, warnings);
}

/*!
 * \brief Returns list of selected nodes for a view
 */
QList<ModelNode> Model::selectedNodes(AbstractView *view) const
{
    return d->toModelNodeList(d->selectedNodes(), view);
}

void Model::setSelectedModelNodes(const QList<ModelNode> &selectedNodeList)
{
    QList<ModelNode> unlockedNodes;

    for (const auto &modelNode : selectedNodeList) {
        if (!ModelUtils::isThisOrAncestorLocked(modelNode))
            unlockedNodes.push_back(modelNode);
    }

    d->setSelectedNodes(toInternalNodeList<Internal::InternalNode::FewNodes>(unlockedNodes));
}

void Model::clearMetaInfoCache()
{
    d->m_nodeMetaInfoCache.clear();
}

/*!
  \brief Returns the URL against which relative URLs within the model should be resolved.
  \return The base URL.
  */
const QUrl &Model::fileUrl() const
{
    return d->fileUrl();
}

SourceId Model::fileUrlSourceId() const
{
    return d->m_sourceId;
}

/*!
  \brief Sets the URL against which relative URLs within the model should be resolved.
  \param url the base URL, i.e. the qml file path.
  */
void Model::setFileUrl(const QUrl &url)
{
    Internal::WriteLocker locker(d.get());
    d->setFileUrl(url);
}

bool Model::hasNodeMetaInfo(const TypeName &typeName, int majorVersion, int minorVersion) const
{
    return metaInfo(typeName, majorVersion, minorVersion).isValid();
}

#ifndef QDS_USE_PROJECTSTORAGE
const MetaInfo Model::metaInfo() const
{
    return d->metaInfo();
}
void Model::setMetaInfo(const MetaInfo &metaInfo)
{
    d->setMetaInfo(metaInfo);
}
#endif

NodeMetaInfo Model::boolMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QML, BoolType>();
    } else {
        return metaInfo("QML.bool");
    }
}

NodeMetaInfo Model::doubleMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QML, DoubleType>();
    } else {
        return metaInfo("QML.double");
    }
}

template<const auto &moduleName, const auto &typeName>
NodeMetaInfo Model::createNodeMetaInfo() const
{
    auto typeId = d->projectStorage->commonTypeCache().typeId<moduleName, typeName>();

    return {typeId, d->projectStorage};
}

NodeMetaInfo Model::fontMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, font>();
    } else {
        return metaInfo("QtQuick.font");
    }
}

NodeMetaInfo Model::qtQmlModelsListModelMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQml_Models, ListModel>();
    } else {
        return metaInfo("QtQml.Models.ListModel");
    }
}

NodeMetaInfo Model::qtQmlModelsListElementMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQml_Models, ListElement>();
    } else {
        return metaInfo("QtQml.Models.ListElement");
    }
}

NodeMetaInfo Model::qtQmlXmlListModelXmlListModelRoleMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQml_XmlListModel, XmlListModelRole>();
    } else {
        return metaInfo("QtQml.XmlListModel.XmlListModelRole");
    }
}

NodeMetaInfo Model::qmlQtObjectMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QML, QtObject>();
    } else {
        return metaInfo("QML.QtObject");
    }
}

NodeMetaInfo Model::qtQuickItemMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Item>();
    } else {
        return metaInfo("QtQuick.Item");
    }
}

NodeMetaInfo Model::qtQuickRectangleMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Rectangle>();
    } else {
        return metaInfo("QtQuick.Rectangle");
    }
}

NodeMetaInfo Model::qtQuickImageMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Image>();
    } else {
        return metaInfo("QtQuick.Image");
    }
}

NodeMetaInfo Model::qtQuickTextMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Text>();
    } else {
        return metaInfo("QtQuick.Text");
    }
}

NodeMetaInfo Model::qtQuickPropertyAnimationMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, PropertyAnimation>();
    } else {
        return metaInfo("QtQuick.PropertyAnimation");
    }
}

NodeMetaInfo Model::qtQuickPropertyChangesMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, PropertyChanges>();
    } else {
        return metaInfo("QtQuick.PropertyChanges");
    }
}

NodeMetaInfo Model::flowViewFlowActionAreaMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowActionArea>();
    } else {
        return metaInfo("FlowView.FlowActionArea");
    }
}

NodeMetaInfo Model::flowViewFlowDecisionMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowDecision>();
    } else {
        return metaInfo("FlowView.FlowDecision");
    }
}

NodeMetaInfo Model::flowViewFlowItemMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowItem>();
    } else {
        return metaInfo("FlowView.FlowItem");
    }
}

NodeMetaInfo Model::flowViewFlowWildcardMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowWildcard>();
    } else {
        return metaInfo("FlowView.FlowWildcard");
    }
}

NodeMetaInfo Model::flowViewFlowTransitionMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowTransition>();
    } else {
        return metaInfo("FlowView.FlowTransition");
    }
}

NodeMetaInfo Model::qtQuickTextEditMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, TextEdit>();
    } else {
        return metaInfo("QtQuick.TextEdit");
    }
}

NodeMetaInfo Model::qtQuickControlsLabelMetaInfo() const
{
#ifdef QDS_USE_PROJECTSTORAGE
    using namespace Storage::Info;
    return createNodeMetaInfo<QtQuick_Controls, Label>();
#else
    return metaInfo("QtQuick.Controls.Label");
#endif
}

NodeMetaInfo Model::qtQuickControlsTextAreaMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Controls, TextArea>();
    } else {
        return metaInfo("QtQuick.Controls.TextArea");
    }
}

NodeMetaInfo Model::qtQuick3DNodeMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Node>();
    } else {
        return metaInfo("QtQuick3D.Node");
    }
}

NodeMetaInfo Model::qtQuick3DPointLightMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, PointLight>();
    } else {
        return metaInfo("QtQuick3D.PointLight");
    }
}

NodeMetaInfo Model::qtQuick3DSpotLightMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, SpotLight>();
    } else {
        return metaInfo("QtQuick3D.SpotLight");
    }
}

NodeMetaInfo Model::qtQuick3DOrthographicCameraMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, OrthographicCamera>();
    } else {
        return metaInfo("QtQuick3D.OrthographicCamera");
    }
}

NodeMetaInfo Model::qtQuick3DParticles3DSpriteParticle3DMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D_Particles3D, SpriteParticle3D>();
    } else {
        return metaInfo("QtQuick3D.Particles3D.SpriteParticle3D");
    }
}

NodeMetaInfo Model::qtQuick3DPerspectiveCameraMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, PerspectiveCamera>();
    } else {
        return metaInfo("QtQuick3D.PerspectiveCamera");
    }
}
NodeMetaInfo Model::qtQuick3DTextureMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Texture>();
    } else {
        return metaInfo("QtQuick3D.Texture");
    }
}

NodeMetaInfo Model::qtQuick3DTextureInputMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, TextureInput>();
    } else {
        return metaInfo("QtQuick3D.TextureInput");
    }
}

NodeMetaInfo Model::qtQuickBorderImageMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, BorderImage>();
    } else {
        return metaInfo("QtQuick.BorderImage");
    }
}

NodeMetaInfo Model::qtQuick3DBakedLightmapMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, BakedLightmap>();
    } else {
        return metaInfo("QtQuick3D.BakedLightmap");
    }
}

NodeMetaInfo Model::qtQuick3DMaterialMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Material>();
    } else {
        return metaInfo("QtQuick3D.Material");
    }
}

NodeMetaInfo Model::qtQuick3DDefaultMaterialMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, DefaultMaterial>();
    } else {
        return metaInfo("QtQuick3D.DefaultMaterial");
    }
}

NodeMetaInfo Model::qtQuick3DDirectionalLightMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, DirectionalLight>();
    } else {
        return metaInfo("QtQuick3D.DirectionalLight");
    }
}

NodeMetaInfo Model::qtQuick3DPrincipledMaterialMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, PrincipledMaterial>();
    } else {
        return metaInfo("QtQuick3D.PrincipledMaterial");
    }
}

NodeMetaInfo Model::qtQuick3DSceneEnvironmentMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, SceneEnvironment>();
    } else {
        return metaInfo("QtQuick3D.SceneEnvironment");
    }
}

NodeMetaInfo Model::qtQuickTimelineTimelineMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Timeline, Timeline>();
    } else {
        return metaInfo("QtQuick.Timeline.Timeline");
    }
}

NodeMetaInfo Model::qtQuickTransistionMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Transition>();
    } else {
        return metaInfo("QtQuick.Transition");
    }
}

NodeMetaInfo Model::qtQmlConnectionsMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQml, Connections>();
    } else {
        return metaInfo("QtQml.Connections");
    }
}

NodeMetaInfo Model::qtQuickStateGroupMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, StateGroup>();
    } else {
        return metaInfo("QtQuick.StateGroup");
    }
}

NodeMetaInfo Model::vector2dMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, vector2d>();
    } else {
        return metaInfo("QtQuick.vector2d");
    }
}

NodeMetaInfo Model::vector3dMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, vector3d>();
    } else {
        return metaInfo("QtQuick.vector3d");
    }
}

NodeMetaInfo Model::vector4dMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, vector4d>();
    } else {
        return metaInfo("QtQuick.vector4d");
    }
}

#ifdef QDS_USE_PROJECTSTORAGE

QVarLengthArray<NodeMetaInfo, 256> Model::metaInfosForModule(Module module) const
{
        using namespace Storage::Info;

        return Utils::transform<QVarLengthArray<NodeMetaInfo, 256>>(
            d->projectStorage->typeIds(module.id()), NodeMetaInfo::bind(d->projectStorage));
}

SmallNodeMetaInfos<256> Model::singletonMetaInfos() const
{
    using namespace Storage::Info;

    return Utils::transform<QVarLengthArray<NodeMetaInfo, 256>>(d->projectStorage->singletonTypeIds(
                                                                    d->m_sourceId),
                                                                NodeMetaInfo::bind(d->projectStorage));
}

#endif

QList<ItemLibraryEntry> Model::itemLibraryEntries() const
{
#ifdef QDS_USE_PROJECTSTORAGE
    using namespace Storage::Info;
    return toItemLibraryEntries(*d->pathCache, d->projectStorage->itemLibraryEntries(d->m_sourceId));
#else
    return d->metaInfo().itemLibraryInfo()->entries();
#endif
}

NodeMetaInfo Model::qtQuickTimelineKeyframeGroupMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Timeline, KeyframeGroup>();
    } else {
        return metaInfo("QtQuick.Timeline.KeyframeGroup");
    }
}

NodeMetaInfo Model::qtQuick3DModelMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Storage::Info::Model>();
    } else {
        return metaInfo("QtQuick3D.Model");
    }
}

namespace {
[[maybe_unused]] std::pair<Utils::SmallStringView, Utils::SmallStringView> moduleTypeName(
    const TypeName &typeName)
{
    auto foundDot = std::find(typeName.begin(), typeName.end(), '.');

    if (foundDot == typeName.end())
        return {"", typeName};

    return {{typeName.begin(), foundDot}, {std::next(foundDot), typeName.end()}};
}
} // namespace

NodeMetaInfo Model::metaInfo(const TypeName &typeName,
                             [[maybe_unused]] int majorVersion,
                             [[maybe_unused]] int minorVersion) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    return NodeMetaInfo(d->projectStorage->typeId(d->importedTypeNameId(typeName)), d->projectStorage);
#else
    return NodeMetaInfo(metaInfoProxyModel(), typeName, majorVersion, minorVersion);
#endif
}

NodeMetaInfo Model::metaInfo([[maybe_unused]] Module module,
                             [[maybe_unused]] Utils::SmallStringView typeName,
                             [[maybe_unused]] Storage::Version version) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    return NodeMetaInfo(d->projectStorage->typeId(module.id(), typeName, version), d->projectStorage);
#else
    return {};
#endif
}

#ifndef QDS_USE_PROJECTSTORAGE
MetaInfo Model::metaInfo()
{
    return d->metaInfo();
}
#endif

Module Model::module(Utils::SmallStringView moduleName, Storage::ModuleKind moduleKind)
{
    if constexpr (useProjectStorage())
        return Module(d->projectStorage->moduleId(moduleName, moduleKind), d->projectStorage);

    return {};
}

/*! \name View related functions
*/
//\{
/*!
\brief Attaches a view to the model

Registers a "view" that from then on will be informed about changes to the model. Different views
will always be informed in the order in which they registered to the model.

The view is informed that it has been registered within the model by a call to AbstractView::modelAttached .

\param view The view object to register. Must be not null.
\see detachView
*/
void Model::attachView(AbstractView *view)
{
    auto traceToken = d->traceToken.begin("attachView",
                                          keyValue("name",
                                                   std::string_view{view->metaObject()->className()}));

    //    Internal::WriteLocker locker(d);
    if (view->kind() == AbstractView::Kind::Rewriter) {
        auto castedRewriterView = qobject_cast<RewriterView *>(view);
        if (rewriterView() == castedRewriterView)
            return;

        setRewriterView(castedRewriterView);
        return;
    }

    if (view->kind() == AbstractView::Kind::NodeInstance)
        return;

    d->attachView(view);
}

/*!
\brief Detaches a view to the model

\param view The view to unregister. Must be not null.
\param emitDetachNotify If set to NotifyView (the default), AbstractView::modelAboutToBeDetached() will be called

\see attachView
*/
void Model::detachView(AbstractView *view, ViewNotification emitDetachNotify)
{
    auto traceToken = d->traceToken.begin("detachView",
                                          keyValue("name",
                                                   std::string_view{view->metaObject()->className()}));

    //    Internal::WriteLocker locker(d);
    bool emitNotify = (emitDetachNotify == NotifyView);

    if (view->kind() != AbstractView::Kind::Other)
        return;

    d->detachView(view, emitNotify);
}

QList<ModelNode> Model::allModelNodesUnordered()
{
    return toModelNodeList(d->allNodesUnordered(), this);
}

ModelNode Model::rootModelNode()
{
    return ModelNode{d->rootNode(), this, nullptr};
}

ModelNode Model::modelNodeForId(const QString &id)
{
    return ModelNode(d->nodeForId(id), this, nullptr);
}

QHash<QStringView, ModelNode> Model::idModelNodeDict()
{
    QHash<QStringView, ModelNode> idModelNodes;

    for (const auto &[key, internalNode] : d->m_idNodeHash.asKeyValueRange())
        idModelNodes.insert(key, ModelNode(internalNode, this, nullptr));

    return idModelNodes;
}

namespace {
ModelNode createNode(Model *model,
                     Internal::ModelPrivate *d,
                     const TypeName &typeName,
                     int majorVersion,
                     int minorVersion)
{
    return ModelNode(d->createNode(typeName, majorVersion, minorVersion, {}, {}, {}, {}, {}),
                     model,
                     nullptr);
}
} // namespace

ModelNode Model::createModelNode(const TypeName &typeName)
{
#ifdef QDS_USE_PROJECTSTORAGE
    return createNode(this, d.get(), typeName, -1, -1);
#else
    const NodeMetaInfo m = metaInfo(typeName);
    return createNode(this, d.get(), typeName, m.majorVersion(), m.minorVersion());
#endif
}

void Model::changeRootNodeType(const TypeName &type)
{
    Internal::WriteLocker locker(this);

    d->changeRootNodeType(type, -1, -1);
}

void Model::removeModelNodes(ModelNodes nodes, BypassModelResourceManagement bypass)
{
    nodes.removeIf(std::logical_not{});

    if (nodes.empty())
        return;

    std::sort(nodes.begin(), nodes.end());

    ModelResourceSet set;

    if (d->m_resourceManagement && bypass == BypassModelResourceManagement::No)
        set = d->m_resourceManagement->removeNodes(std::move(nodes), this);
    else
        set = {std::move(nodes), {}, {}};

    d->handleResourceSet(set);
}

void Model::removeProperties(AbstractProperties properties, BypassModelResourceManagement bypass)
{
    properties.removeIf(std::logical_not{});

    if (properties.empty())
        return;

    std::sort(properties.begin(), properties.end());

    ModelResourceSet set;

    if (d->m_resourceManagement && bypass == BypassModelResourceManagement::No)
        set = d->m_resourceManagement->removeProperties(std::move(properties), this);
    else
        set = {{}, std::move(properties), {}};

    d->handleResourceSet(set);
}

} // namespace QmlDesigner
