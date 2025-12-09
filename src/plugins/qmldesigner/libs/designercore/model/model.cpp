// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "model.h"
#include "internalnode_p.h"
#include "model_p.h"

#include <itemlibraryentry.h>
#include <modelutils.h>
#include <sourcepathstorage/sourcepathcache.h>
#ifndef QDS_USE_PROJECTSTORAGE
#  include "metainfo.h"
#endif

#include "nodelistproperty.h"
#include "rewriterview.h"
#include "rewritingexception.h"
#include "signalhandlerproperty.h"
#include "variantproperty.h"

#include <predicate.h>
#include <uniquename.h>

#include <modulesstorage/modulesstorage.h>
#include <projectstorage/projectstorage.h>
#include <qmldesignerutils/stringutils.h>
#include <qmldesignerutils/version.h>

#include <QWidget>

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

using ModelTracing::category;
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
    , modulesStorage{&projectStorageDependencies.modulesStorage}
    , projectStorageTriggerUpdate{&projectStorageDependencies.triggerUpdate}
    , m_model{model}
    , m_resourceManagement{std::move(resourceManagement)}
{
    NanotraceHR::Tracer tracer{"model private constructor", category()};

    m_metaInfoProxyModel = metaInfoProxyModel;

    m_imports = {Import::createLibraryImport({"QtQuick"})};

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
    , modulesStorage{&projectStorageDependencies.modulesStorage}
    , projectStorageTriggerUpdate{&projectStorageDependencies.triggerUpdate}
    , m_model{model}
    , m_resourceManagement{std::move(resourceManagement)}
{
    NanotraceHR::Tracer tracer{"model private constructor", category()};

    setFileUrl(fileUrl);
    m_imports = std::move(imports);

    m_rootInternalNode = createNode(typeName, -1, -1, {}, {}, {}, ModelNode::NodeWithoutSource, {}, true);

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
    NanotraceHR::Tracer tracer{"model private constructor", category()};

    m_metaInfoProxyModel = metaInfoProxyModel;

    m_rootInternalNode = createNode(
        typeName, major, minor, {}, {}, {}, ModelNode::NodeWithoutSource, {}, true);

    m_currentStateNode = m_rootInternalNode;
    m_currentTimelineNode = m_rootInternalNode;
}

ModelPrivate::~ModelPrivate()
{
    NanotraceHR::Tracer tracer{"model private destructor", category()};

    removeNode(rootNode());
    if constexpr (useProjectStorage())
        projectStorage->removeObserver(this);
};

void ModelPrivate::detachAllViews()
{
    NanotraceHR::Tracer _{"model private detach all views", category()};

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
auto createModuleId(const Import &import,
                    Utils::SmallStringView localDirectoryPath,
                    ModulesStorage &modulesStorage)
{
    using Storage::ModuleKind;

    auto moduleKind = import.isLibraryImport() ? ModuleKind::QmlLibrary : ModuleKind::PathLibrary;

    if (moduleKind == ModuleKind::QmlLibrary)
        return modulesStorage.moduleId(Utils::SmallString{import.url()}, moduleKind);

    auto path = Utils::PathString::join({localDirectoryPath, "/", Utils::SmallString{import.file()}});
    auto normalizedPath = std::filesystem::path{std::string_view{path}}.lexically_normal();
    auto directoryPath = normalizedPath.generic_string();
    std::string_view modulePath = directoryPath;
    if (modulePath.back() == '/')
        modulePath.remove_suffix(1);

    return modulesStorage.moduleId(modulePath, moduleKind);
}

namespace {

void removeDuplicates(std::ranges::range auto &entries)
{
    std::ranges::sort(entries);
    auto removed = std::ranges::unique(entries);
    entries.erase(removed.begin(), removed.end());
}

} // namespace

Storage::Imports createStorageImports(const Imports &imports,
                                      Utils::SmallStringView localDirectoryPath,
                                      ModulesStorage &modulesStorage,
                                      SourceId fileId)
{
    using Storage::ModuleKind;
    Storage::Imports storageImports;
    storageImports.reserve(Utils::usize(imports) + 1);

    for (const Import &import : imports) {
        ModuleId moduleId = createModuleId(import, localDirectoryPath, modulesStorage);
        storageImports.emplace_back(moduleId,
                                    Storage::Version::convertFromSignedInteger(import.majorVersion(),
                                                                               import.minorVersion()),
                                    fileId,
                                    fileId,
                                    import.alias());
    }

    auto localDirectoryModuleId = modulesStorage.moduleId(localDirectoryPath, ModuleKind::PathLibrary);
    storageImports.emplace_back(localDirectoryModuleId, Storage::Version{}, fileId, fileId);

    auto qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    storageImports.emplace_back(qmlModuleId, Storage::Version{}, fileId, fileId);

    removeDuplicates(storageImports);

    return storageImports;
}

} // namespace

void ModelPrivate::changeImports(Imports toBeAddedImports, Imports toBeRemovedImports)
{
    NanotraceHR::Tracer tracer{"model private change imports",
                               category(),
                               keyValue("added imports", toBeAddedImports),
                               keyValue("removed imports", toBeRemovedImports)};

    std::ranges::sort(toBeAddedImports);
    std::ranges::sort(toBeRemovedImports);

    Imports removedImports = set_intersection(m_imports, toBeRemovedImports);
    m_imports = set_difference(m_imports, removedImports);

    Imports allNewAddedImports = set_strict_difference(toBeAddedImports, m_imports);
    Imports importWithoutAddedImport = set_difference(m_imports, allNewAddedImports);

    m_imports = set_union(importWithoutAddedImport, allNewAddedImports);

    if (!removedImports.isEmpty() || !allNewAddedImports.isEmpty()) {
        if (useProjectStorage()) {
            auto imports = createStorageImports(m_imports, m_localPath, *modulesStorage, m_sourceId);
            projectStorage->synchronizeDocumentImports(std::move(imports), m_sourceId);
        }
        notifyImportsChanged(allNewAddedImports, removedImports);
        updateModelNodeTypeIds(removedImports);
    }
}

void ModelPrivate::setImports(Imports imports)
{
    NanotraceHR::Tracer tracer{"model private set imports",
                               category(),
                               keyValue("imports", imports),
                               keyValue("source id", m_sourceId)};

    std::ranges::sort(imports);

    Imports removedImports = set_strict_difference(m_imports, imports);
    Imports addedImports = set_strict_difference(imports, m_imports);

    m_imports = imports;

    if (!removedImports.isEmpty() || !addedImports.isEmpty()) {
        if (useProjectStorage()) {
            auto imports = createStorageImports(m_imports, m_localPath, *modulesStorage, m_sourceId);
            projectStorage->synchronizeDocumentImports(std::move(imports), m_sourceId);
        }
        notifyImportsChanged(addedImports, removedImports);
        updateModelNodeTypeIds(removedImports);
    }
}

void ModelPrivate::notifyImportsChanged(const Imports &addedImports, const Imports &removedImports)
{
    NanotraceHR::Tracer tracer{"model private notify imports changed", category()};

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
    NanotraceHR::Tracer tracer{"model private notify possible imports changed", category()};

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        Q_ASSERT(view != nullptr);
        view->possibleImportsChanged(possibleImports);
    }
}

void ModelPrivate::notifyUsedImportsChanged(const Imports &usedImports)
{
    NanotraceHR::Tracer tracer{"model private notify used imports changed", category()};

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        Q_ASSERT(view != nullptr);
        view->usedImportsChanged(usedImports);
    }
}

const QUrl &ModelPrivate::fileUrl() const
{
    NanotraceHR::Tracer tracer{"model private file url", category()};

    return m_fileUrl;
}

void ModelPrivate::setDocumentMessages(const QList<DocumentMessage> &errors,
                                       const QList<DocumentMessage> &warnings)
{
    NanotraceHR::Tracer tracer{"model private set document messages", category()};

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
        view->documentMessagesChanged(errors, warnings);
}

void ModelPrivate::setFileUrl(const QUrl &fileUrl)
{
    NanotraceHR::Tracer _{"model private set file url", category(), keyValue("file url", fileUrl)};

    auto tracer = traceToken.begin("file url");

    QUrl oldPath = m_fileUrl;

    if (oldPath != fileUrl) {
        m_fileUrl = fileUrl;
        if constexpr (useProjectStorage()) {
            auto path = fileUrl.toString(QUrl::PreferLocalFile);
            m_sourceId = pathCache->sourceId(SourcePath{path});
            auto found = std::find(path.rbegin(), path.rend(), u'/').base();
            m_localPath = Utils::PathString{QStringView{path.begin(), std::prev(found)}};
        }

        for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
            view->fileUrlChanged(oldPath, fileUrl);
    }
}

void ModelPrivate::changeNodeType(const InternalNodePointer &node,
                                  const TypeName &typeName,
                                  int majorVersion,
                                  int minorVersion)
{
    NanotraceHR::Tracer tracer{"model private change node type",
                               category(),
                               keyValue("node", *node),
                               keyValue("type name", typeName)};

    auto [alias, unqualifiedTypeName] = StringUtils::split_last(typeName);

    node->typeName = typeName;
    node->unqualifiedTypeName = unqualifiedTypeName;
    node->majorVersion = majorVersion;
    node->minorVersion = minorVersion;
    setTypeId(node.get(), alias, unqualifiedTypeName);

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
    NanotraceHR::Tracer tracer{"model private create node",
                               category(),
                               keyValue("type name", typeName)};

    if (typeName.isEmpty())
        return {};

    qint32 internalId = 0;

    if (!isRootNode)
        internalId = m_internalIdCounter++;

    auto [alias, unqualifiedTypeName] = StringUtils::split_last(typeName);

    auto newNode = std::make_shared<InternalNode>(typeName,
                                                  unqualifiedTypeName,
                                                  majorVersion,
                                                  minorVersion,
                                                  internalId,
                                                  traceToken.tickWithFlow("create node"));

    setTypeId(newNode.get(), alias, unqualifiedTypeName);

    newNode->nodeSourceType = nodeSourceType;

    newNode->behaviorPropertyName = behaviorPropertyName;

    using PropertyPair = QPair<PropertyName, QVariant>;

    for (const PropertyPair &propertyPair : propertyList) {
        newNode->getVariantProperty(propertyPair.first);
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
    NanotraceHR::Tracer tracer{"model private remove node from model",
                               category(),
                               keyValue("node", *node)};

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

ImportedTypeNameId ModelPrivate::importedTypeNameId(Utils::SmallStringView typeName)
{
    NanotraceHR::Tracer tracer{"model private imported type name id",
                               category(),
                               keyValue("type name", typeName)};

    auto [moduleName, unqualifiedTypeName] = StringUtils::split_last(typeName);

    return importedTypeNameId(moduleName, unqualifiedTypeName);
}

ImportedTypeNameId ModelPrivate::importedTypeNameId(Utils::SmallStringView alias,
                                                    Utils::SmallStringView unqualifiedTypeName)
{
    NanotraceHR::Tracer tracer{"model private imported type name id",
                               category(),
                               keyValue("alias", alias),
                               keyValue("unqualified type name", unqualifiedTypeName)};

    if (alias.size()) {
        ImportId importId = projectStorage->importId(m_sourceId, alias);
        return projectStorage->importedTypeNameId(importId, unqualifiedTypeName);
    }

    return projectStorage->importedTypeNameId(m_sourceId, unqualifiedTypeName);
}

void ModelPrivate::setTypeId(InternalNode *node,
                             Utils::SmallStringView alias,
                             Utils::SmallStringView unqualifiedTypeName)
{
    NanotraceHR::Tracer tracer{"model private set type id",
                               category(),
                               keyValue("alias", alias),
                               keyValue("unqualified type name", unqualifiedTypeName),
                               keyValue("old exported type", node->exportedTypeName)};

    if constexpr (useProjectStorage()) {
        node->importedTypeNameId = importedTypeNameId(alias, unqualifiedTypeName);
        node->exportedTypeName = projectStorage->exportedTypeName(node->importedTypeNameId);
        tracer.end(keyValue("new exported type", node->exportedTypeName),
                   keyValue("imported type name id", node->importedTypeNameId));
    }
}

bool ModelPrivate::refreshExportedTypeName(InternalNode *node)
{
    NanotraceHR::Tracer tracer{"model private refresh exported type name",
                               category(),
                               keyValue("node", *node)};

    if constexpr (useProjectStorage()) {
        auto exportedTypeName = projectStorage->exportedTypeName(node->importedTypeNameId);
        if (node->exportedTypeName != exportedTypeName) {
            node->exportedTypeName = exportedTypeName;
            tracer.end(keyValue("refreshed exported type name", node->exportedTypeName));
            return true;
        }
    }

    return false;
}

void ModelPrivate::handleResourceSet(const ModelResourceSet &resourceSet)
{
    NanotraceHR::Tracer tracer{"model private handle resource set", category()};

    for (const ModelNode &node : resourceSet.removeModelNodes) {
        if (node)
            removeNode(node.m_internalNode);
    }

    removeProperties(toInternalProperties(resourceSet.removeProperties));

    setBindingProperties(resourceSet.setExpressions);
}

void ModelPrivate::updateModelNodeTypeIds(const ExportedTypeNames &addedExportedTypeNames,
                                          const ExportedTypeNames &removedExportedTypeNames)
{
    NanotraceHR::Tracer tracer{"model private update model node type ids", category()};

    auto nodes = m_nodes;
    std::ranges::sort(nodes, {}, &InternalNode::exportedTypeName);

    bool rootNodeIsRefreshed = false;
    QVarLengthArray<InternalNodePointer, 24> refreshedNodes;

    auto refeshNodeTypeId = [&](auto &node) {
        bool isRefreshed = refreshExportedTypeName(node.get());
        if (isRefreshed) {
            if (node == m_rootInternalNode)
                rootNodeIsRefreshed = true;
            else
                refreshedNodes.push_back(node);
        }
    };
    Utils::set_greedy_intersection(nodes,
                                   removedExportedTypeNames,
                                   refeshNodeTypeId,
                                   {},
                                   &InternalNode::exportedTypeName);

    std::ranges::sort(nodes, {}, &InternalNode::unqualifiedTypeName);

    Utils::set_greedy_intersection(nodes,
                                   addedExportedTypeNames,
                                   refeshNodeTypeId,
                                   {},
                                   &InternalNode::unqualifiedTypeName,
                                   &Storage::Info::ExportedTypeName::name);

    removeDuplicates(refreshedNodes);

    if (rootNodeIsRefreshed) {
        notifyRootNodeTypeChanged(QString::fromUtf8(m_rootInternalNode->typeName),
                                  m_rootInternalNode->exportedTypeName.version.major.toSignedInteger(),
                                  m_rootInternalNode->exportedTypeName.version.minor.toSignedInteger());
    }

    for (const auto &node : refreshedNodes) {
        notifyNodeTypeChanged(node,
                              node->typeName,
                              node->exportedTypeName.version.major.toSignedInteger(),
                              node->exportedTypeName.version.minor.toSignedInteger());
    }
}

void ModelPrivate::updateModelNodeTypeIds(const Imports &removedImports)
{
    auto removedModuleIds = Utils::transform<SmallModuleIds<24>>(removedImports, [&](const Import &import) {
        return createModuleId(import, m_localPath, *modulesStorage);
    });

    bool rootNodeIsRefreshed = false;
    QVarLengthArray<InternalNodePointer, 24> refreshedNodes;

    auto refeshNodeTypeId = [&](auto &node) {
        bool isRefreshed = refreshExportedTypeName(node.get());
        if (isRefreshed) {
            if (node == m_rootInternalNode)
                rootNodeIsRefreshed = true;
            else
                refreshedNodes.push_back(node);
        }
    };

    for (InternalNodePointer &node : m_nodes) {
        if (removedModuleIds.contains(node->exportedTypeName.moduleId)
            or not node->exportedTypeName.typeId)
            refeshNodeTypeId(node);
    }

    removeDuplicates(refreshedNodes);

    if (rootNodeIsRefreshed) {
        notifyRootNodeTypeChanged(QString::fromUtf8(m_rootInternalNode->typeName),
                                  m_rootInternalNode->exportedTypeName.version.major.toSignedInteger(),
                                  m_rootInternalNode->exportedTypeName.version.minor.toSignedInteger());
    }

    for (const auto &node : refreshedNodes) {
        notifyNodeTypeChanged(node,
                              node->typeName,
                              node->exportedTypeName.version.major.toSignedInteger(),
                              node->exportedTypeName.version.minor.toSignedInteger());
    }
}

void ModelPrivate::removedTypeIds(const TypeIds &removedTypeIds)
{
    NanotraceHR::Tracer tracer{"model private removed type ids", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) { view->refreshMetaInfos(removedTypeIds); });
}

void ModelPrivate::exportedTypeNamesChanged(const ExportedTypeNames &added,
                                            const ExportedTypeNames &removed)
{
    NanotraceHR::Tracer tracer{"model private exported type names changed", category()};

    updateModelNodeTypeIds(added, removed);

    notifyNodeInstanceViewLast(
        [&](AbstractView *view) { view->exportedTypeNamesChanged(added, removed); });
}

void ModelPrivate::removeAllSubNodes(const InternalNodePointer &node)
{
    NanotraceHR::Tracer tracer{"model private remove all sub nodes", category()};

    for (const InternalNodePointer &subNode : node->allSubNodes())
        removeNodeFromModel(subNode);
}

void ModelPrivate::removeNodeAndRelatedResources(const InternalNodePointer &node)
{
    NanotraceHR::Tracer tracer{"model private remove node and related resources", category()};

    if (m_resourceManagement) {
        handleResourceSet(
            m_resourceManagement->removeNodes({ModelNode{node, m_model, nullptr}}, m_model));
    } else {
        removeNode(node);
    }
}

void ModelPrivate::removeNode(const InternalNodePointer &node)
{
    NanotraceHR::Tracer tracer{"model private remove node", category()};

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
    NanotraceHR::Tracer tracer{"model private root node", category()};

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
    NanotraceHR::Tracer tracer{"model private change node id",
                               category(),
                               keyValue("node", *node),
                               keyValue("id", id)};

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
    NanotraceHR::Tracer tracer{"model private property name is valid",
                               category(),
                               keyValue("property name", propertyName)};
    if (propertyName.isEmpty())
        return false;

    if (propertyName == "id")
        return false;

    return true;
}

void ModelPrivate::notifyNodeInstanceViewLast(const std::invocable<AbstractView *> auto &call)
{
    NanotraceHR::Tracer tracer{"model private notify node instance view last", category()};

    bool resetModel = false;
    QString description;

    try {
        if (m_rewriterView && !m_rewriterView->isBlockingNotifications()) {
            NanotraceHR::Tracer tracer{m_rewriterView->name(), category()};
            call(m_rewriterView);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        try {
            if (!view->isBlockingNotifications()) {
                NanotraceHR::Tracer tracer{view->name(), category()};
                call(view.data());
            }
        } catch (const Exception &e) {
            e.showException(tr("Exception thrown by view %1.").arg(view->widgetInfo().tabName));
        }
    }

    if (m_nodeInstanceView && !m_nodeInstanceView->isBlockingNotifications()) {
        NanotraceHR::Tracer tracer{m_nodeInstanceView->name(), category()};
        call(m_nodeInstanceView);
    }

    if (resetModel) {
        try {
            resetModelByRewriter(description);
        } catch (const RewritingException &e) {
            e.showException();
        }
    }
}

void ModelPrivate::notifyNormalViewsLast(const std::invocable<AbstractView *> auto &call)
{
    NanotraceHR::Tracer tracer{"model private notify normal views last", category()};

    bool resetModel = false;
    QString description;

    try {
        if (m_rewriterView && !m_rewriterView->isBlockingNotifications()) {
            NanotraceHR::Tracer tracer{m_rewriterView->name(), category()};
            call(m_rewriterView);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (m_nodeInstanceView && !m_nodeInstanceView->isBlockingNotifications()) {
        NanotraceHR::Tracer tracer{m_nodeInstanceView->name(), category()};
        call(m_nodeInstanceView);
    }

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        if (!view->isBlockingNotifications()) {
            NanotraceHR::Tracer tracer{view->name(), category()};
            call(view.data());
        }
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyInstanceChanges(const std::invocable<AbstractView *> auto &call)
{
    NanotraceHR::Tracer tracer{"model private notify instance changes", category()};

    for (const QPointer<AbstractView> &view : std::as_const(m_viewList)) {
        if (!view->isBlockingNotifications()) {
            NanotraceHR::Tracer tracer{view->name(), category()};
            call(view.data());
        }
    }
}

void ModelPrivate::notifyAuxiliaryDataChanged(const InternalNodePointer &node,
                                              AuxiliaryDataKeyView key,
                                              const QVariant &data)
{
    NanotraceHR::Tracer tracer{"model private notify auxiliary data changed", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        ModelNode modelNode(node, m_model, view);
        view->auxiliaryDataChanged(modelNode, key, data);
    });
}

void ModelPrivate::notifyNodeSourceChanged(const InternalNodePointer &node,
                                           const QString &newNodeSource)
{
    NanotraceHR::Tracer tracer{"model private notify node source changed", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        ModelNode ModelNode(node, m_model, view);
        view->nodeSourceChanged(ModelNode, newNodeSource);
    });
}

void ModelPrivate::notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    NanotraceHR::Tracer tracer{"model private notify root node type changed", category()};

    notifyNodeInstanceViewLast(
        [&](AbstractView *view) { view->rootNodeTypeChanged(type, majorVersion, minorVersion); });
}

void ModelPrivate::notifyInstancePropertyChange(Utils::span<const QPair<ModelNode, PropertyName>> properties)
{
    NanotraceHR::Tracer tracer{"model private notify instance property change", category()};

    notifyInstanceChanges([&](AbstractView *view) {
        using ModelNodePropertyPair = QPair<ModelNode, PropertyName>;
        QList<QPair<ModelNode, PropertyName>> adaptedPropertyList;
        for (const ModelNodePropertyPair &propertyPair : properties) {
            ModelNodePropertyPair newPair(ModelNode{propertyPair.first.internalNode(), m_model, view},
                                          propertyPair.second);
            adaptedPropertyList.append(newPair);
        }
        view->instancePropertyChanged(adaptedPropertyList);
    });
}

void ModelPrivate::notifyInstanceErrorChange(Utils::span<const qint32> instanceIds)
{
    NanotraceHR::Tracer tracer{"model private notify instance error change", category()};

    notifyInstanceChanges([&](AbstractView *view) {
        QVector<ModelNode> errorNodeList;
        errorNodeList.reserve(std::ssize(instanceIds));
        for (qint32 instanceId : instanceIds)
            errorNodeList.emplace_back(m_model->d->nodeForInternalId(instanceId), m_model, view);
        view->instanceErrorChanged(errorNodeList);
    });
}

void ModelPrivate::notifyInstancesCompleted(Utils::span<const ModelNode> modelNodes)
{
    NanotraceHR::Tracer tracer{"model private notify instances completed", category()};

    auto internalNodes = toInternalNodeList(modelNodes);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesCompleted(toModelNodeList(internalNodes, view));
    });
}

namespace {
QMultiHash<ModelNode, InformationName> convertModelNodeInformationHash(
    const QMultiHash<ModelNode, InformationName> &informationChangeHash, AbstractView *view)
{
    NanotraceHR::Tracer tracer{"model private convert model node information hash", category()};

    QMultiHash<ModelNode, InformationName> convertedModelNodeInformationHash;

    for (auto it = informationChangeHash.cbegin(), end = informationChangeHash.cend(); it != end; ++it)
        convertedModelNodeInformationHash.insert(ModelNode(it.key(), view), it.value());

    return convertedModelNodeInformationHash;
}
} // namespace

void ModelPrivate::notifyInstancesInformationsChange(
    const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    NanotraceHR::Tracer tracer{"model private notify instances informations change", category()};

    notifyInstanceChanges([&](AbstractView *view) {
        view->instanceInformationsChanged(convertModelNodeInformationHash(informationChangeHash, view));
    });
}

void ModelPrivate::notifyInstancesRenderImageChanged(Utils::span<const ModelNode> nodes)
{
    NanotraceHR::Tracer tracer{"model private notify instance render image changed", category()};

    auto internalNodes = toInternalNodeList(nodes);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesRenderImageChanged(toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyInstancesPreviewImageChanged(Utils::span<const ModelNode> nodes)
{
    NanotraceHR::Tracer tracer{"model private notify instances preview image changed", category()};

    auto internalNodes = toInternalNodeList(nodes);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesPreviewImageChanged(toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyInstancesChildrenChanged(Utils::span<const ModelNode> nodes)
{
    NanotraceHR::Tracer tracer{"model private notify instances children changed", category()};

    auto internalNodes = toInternalNodeList(nodes);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesChildrenChanged(toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyCurrentStateChanged(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"model private notify current state changed", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->currentStateChanged(ModelNode(node.internalNode(), m_model, view));
    });
}

void ModelPrivate::notifyCurrentTimelineChanged(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"model private notify current timeline changed", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->currentTimelineChanged(ModelNode(node.internalNode(), m_model, view));
    });
}

void ModelPrivate::notifyRenderImage3DChanged(const QImage &image)
{
    NanotraceHR::Tracer tracer{"model private notify render image 3d changed", category()};

    notifyInstanceChanges([&](AbstractView *view) { view->renderImage3DChanged(image); });
}

void ModelPrivate::notifyUpdateActiveScene3D(const QVariantMap &sceneState)
{
    NanotraceHR::Tracer tracer{"model private notify update active scene 3d", category()};

    notifyInstanceChanges([&](AbstractView *view) { view->updateActiveScene3D(sceneState); });
}

void ModelPrivate::notifyModelNodePreviewPixmapChanged(const ModelNode &node,
                                                       const QPixmap &pixmap,
                                                       const QByteArray &requestId)
{
    NanotraceHR::Tracer tracer{"model private notify model node preview pixmap changed", category()};

    notifyInstanceChanges(
        [&](AbstractView *view) { view->modelNodePreviewPixmapChanged(node, pixmap, requestId); });
}

void ModelPrivate::notifyImport3DSupportChanged(const QVariantMap &supportMap)
{
    NanotraceHR::Tracer tracer{"model private notify import 3d support changed", category()};

    notifyInstanceChanges([&](AbstractView *view) { view->updateImport3DSupport(supportMap); });
}

void ModelPrivate::notifyNodeAtPosResult(const ModelNode &modelNode, const QVector3D &pos3d)
{
    NanotraceHR::Tracer tracer{"model private notify node at result", category()};

    notifyInstanceChanges([&](AbstractView *view) { view->nodeAtPosReady(modelNode, pos3d); });
}

void ModelPrivate::notifyView3DAction(View3DActionType type, const QVariant &value)
{
    NanotraceHR::Tracer tracer{"model private notify view 3d action", category()};

    notifyNormalViewsLast([&](AbstractView *view) { view->view3DAction(type, value); });
}

void ModelPrivate::notifyDragStarted(QMimeData *mimeData)
{
    NanotraceHR::Tracer tracer{"model private notify drag started", category()};

    notifyInstanceChanges([&](AbstractView *view) { view->dragStarted(mimeData); });
}

void ModelPrivate::notifyDragEnded()
{
    NanotraceHR::Tracer tracer{"model private notify drag ended", category()};

    notifyInstanceChanges([&](AbstractView *view) { view->dragEnded(); });
}

void ModelPrivate::notifyRewriterBeginTransaction()
{
    NanotraceHR::Tracer tracer{"model private notify rewriter begin transaction", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) { view->rewriterBeginTransaction(); });
}

void ModelPrivate::notifyRewriterEndTransaction()
{
    NanotraceHR::Tracer tracer{"model private notify rewriter end transaction", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) { view->rewriterEndTransaction(); });
}

void ModelPrivate::notifyInstanceToken(const QString &token,
                                       int number,
                                       Utils::span<const ModelNode> nodes)
{
    NanotraceHR::Tracer tracer{"model private notify instance token", category()};

    auto internalNodes = toInternalNodeList(nodes);

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesToken(token, number, toModelNodeList(internalNodes, view));
    });
}

void ModelPrivate::notifyCustomNotification(const AbstractView *senderView,
                                            const QString &identifier,
                                            Utils::span<const ModelNode> nodes,
                                            const QList<QVariant> &data)
{
    NanotraceHR::Tracer tracer{"model private notify custom notification", category()};

    auto internalList = toInternalNodeList(nodes);
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->customNotification(senderView, identifier, toModelNodeList(internalList, view), data);
    });
}

void ModelPrivate::notifyCustomNotificationTo(AbstractView *view,
                                              const CustomNotificationPackage &package)
{
    NanotraceHR::Tracer tracer{"model private notify custom notification to", category()};

    if (view) {
        NanotraceHR::Tracer tracer{view->name(), category()};
        view->customNotification(package);
    }
}

void ModelPrivate::notifyPropertiesRemoved(const QList<PropertyPair> &propertyPairList)
{
    NanotraceHR::Tracer tracer{"model private notify properties removed", category()};

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
    NanotraceHR::Tracer tracer{"model private notify properties about to be removed", category()};

    bool resetModel = false;
    QString description;

    try {
        if (m_rewriterView) {
            NanotraceHR::Tracer tracer{m_rewriterView->name(), category()};

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
        NanotraceHR::Tracer tracer{view->name(), category()};
        QList<AbstractProperty> propertyList;
        Q_ASSERT(view != nullptr);
        for (auto property : internalPropertyList) {
            AbstractProperty newProperty(property->name(),
                                         property->propertyOwner(),
                                         m_model,
                                         view.data());
            propertyList.append(newProperty);
        }

        try {
            view->propertiesAboutToBeRemoved(propertyList);
        } catch (const RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }

    if (m_nodeInstanceView) {
        NanotraceHR::Tracer tracer{m_nodeInstanceView->name(), category()};
        QList<AbstractProperty> propertyList;
        for (auto property : internalPropertyList) {
            AbstractProperty newProperty(property->name(),
                                         property->propertyOwner(),
                                         m_model,
                                         m_nodeInstanceView);
            propertyList.append(newProperty);
        }

        m_nodeInstanceView->propertiesAboutToBeRemoved(propertyList);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::setAuxiliaryData(const InternalNodePointer &node,
                                    const AuxiliaryDataKeyView &key,
                                    const QVariant &data)
{
    NanotraceHR::Tracer tracer{"model private set auxiliary data", category()};

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
    NanotraceHR::Tracer tracer{"model private reset model by rewriter", category()};

    if (m_rewriterView) {
        m_rewriterView->resetToLastCorrectQml();

        throw RewritingException(description, rewriterView()->textModifierContent());
    }
}

void ModelPrivate::attachView(AbstractView *view)
{
    NanotraceHR::Tracer tracer{"model private attach view", category()};

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
    NanotraceHR::Tracer tracer{"model private detach view", category()};

    if (!view->isAttached())
        return;

    if (notifyView)
        view->modelAboutToBeDetached(m_model);
    m_viewList.removeOne(view);
}

void ModelPrivate::notifyNodeCreated(const InternalNodePointer &newInternalNodePointer)
{
    NanotraceHR::Tracer tracer{"model private notify node created", category()};

    notifyNormalViewsLast([&](AbstractView *view) {
        view->nodeCreated(ModelNode{newInternalNodePointer, m_model, view});
    });
}

void ModelPrivate::notifyNodeAboutToBeRemoved(const InternalNodePointer &internalNodePointer)
{
    NanotraceHR::Tracer tracer{"model private notify node about to be removed", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeAboutToBeRemoved(ModelNode{internalNodePointer, m_model, view});
    });
}

void ModelPrivate::notifyNodeRemoved(const InternalNodePointer &removedNode,
                                     const InternalNodePointer &parentNode,
                                     PropertyNameView parentPropertyName,
                                     AbstractView::PropertyChangeFlags propertyChange)
{
    NanotraceHR::Tracer tracer{"model private notify node removed", category()};

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
    NanotraceHR::Tracer tracer{"model private notify node type changed", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeTypeChanged(ModelNode{node, m_model, view}, type, majorVersion, minorVersion);
    });
}

void ModelPrivate::notifyNodeIdChanged(const InternalNodePointer &node,
                                       const QString &newId,
                                       const QString &oldId)
{
    NanotraceHR::Tracer tracer{"model private notify node id changed", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeIdChanged(ModelNode{node, m_model, view}, newId, oldId);
    });
}

void ModelPrivate::notifyBindingPropertiesAboutToBeChanged(
    const QList<InternalBindingProperty *> &internalPropertyList)
{
    NanotraceHR::Tracer tracer{"model private notify binding properties about to be changed",
                               category()};

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
    NanotraceHR::Tracer tracer{"model private notify binding properties changed", category()};

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
    NanotraceHR::Tracer tracer{"model private notify signal handler properties changed", category()};

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
    NanotraceHR::Tracer tracer{"model private notify signal declaration properties changed",
                               category()};

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
    NanotraceHR::Tracer tracer{"model private notify script functions changed", category()};

    notifyNormalViewsLast([&](AbstractView *view) {
        view->scriptFunctionsChanged(ModelNode{node, m_model, view}, scriptFunctionList);
    });
}

void ModelPrivate::notifyVariantPropertiesChanged(const InternalNodePointer &node,
                                                  const PropertyNameViews &propertyNameViews,
                                                  AbstractView::PropertyChangeFlags propertyChange)
{
    NanotraceHR::Tracer tracer{"model private notify variant properties changed", category()};

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
    NanotraceHR::Tracer tracer{"model private notify node about to be reparent", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent && oldParent->isValid)
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, m_model, view);

        if (!newPropertyName.isEmpty() && newParent && newParent->isValid) {
            newProperty = NodeAbstractProperty(newPropertyName, newParent, m_model, view);
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
    NanotraceHR::Tracer tracer{"model private notify node reparent", category()};

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
    NanotraceHR::Tracer tracer{"model private notify node order changed", category()};

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
    NanotraceHR::Tracer tracer{"model private notify node order changed", category()};

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
    NanotraceHR::Tracer tracer{"model private set selected nodes", category()};

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

    notifySelectedNodesChanged(sortedSelectedList, lastSelectedNodeList);
}

void ModelPrivate::clearSelectedNodes()
{
    NanotraceHR::Tracer _{"model private clear selected nodes", category()};

    auto tracer = traceToken.begin("clear selected model nodes");

    auto lastSelectedNodeList = m_selectedInternalNodes;
    m_selectedInternalNodes.clear();
    notifySelectedNodesChanged(m_selectedInternalNodes, lastSelectedNodeList);
}

void ModelPrivate::removeAuxiliaryData(const InternalNodePointer &node, const AuxiliaryDataKeyView &key)
{
    NanotraceHR::Tracer tracer{"model private remove auxiliary data", category()};

    bool removed = node->removeAuxiliaryData(key);

    if (removed)
        notifyAuxiliaryDataChanged(node, key, QVariant());
}

QList<ModelNode> ModelPrivate::toModelNodeList(Utils::span<const InternalNodePointer> nodes,
                                               AbstractView *view) const
{
    NanotraceHR::Tracer tracer{"model private to model node list", category()};

    QList<ModelNode> modelNodeList;
    modelNodeList.reserve(std::ssize(nodes));
    for (const InternalNodePointer &node : nodes)
        modelNodeList.emplace_back(node, m_model, view);

    return modelNodeList;
}

ModelPrivate::ManyNodes ModelPrivate::toInternalNodeList(Utils::span<const ModelNode> modelNodes) const
{
    NanotraceHR::Tracer tracer{"model private to internal node list", category()};

    ManyNodes newNodeList;
    newNodeList.reserve(std::ssize(modelNodes));
    for (const ModelNode &modelNode : modelNodes)
        newNodeList.append(modelNode.internalNode());

    return newNodeList;
}

QList<InternalProperty *> ModelPrivate::toInternalProperties(const AbstractProperties &properties)
{
    NanotraceHR::Tracer tracer{"model private to internal properties", category()};

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

void ModelPrivate::notifySelectedNodesChanged(const FewNodes &newSelectedNodeList,
                                              const FewNodes &oldSelectedNodeList)
{
    NanotraceHR::Tracer tracer{"model private change selected nodes", category()};

    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->selectedNodesChanged(toModelNodeList(newSelectedNodeList, view),
                                   toModelNodeList(oldSelectedNodeList, view));
    });
}

const ModelPrivate::FewNodes &ModelPrivate::selectedNodes() const
{
    NanotraceHR::Tracer tracer{"model private selected nodes", category()};

    static FewNodes empty;
    for (const InternalNodePointer &node : m_selectedInternalNodes) {
        if (!node->isValid)
            return empty;
    }

    return m_selectedInternalNodes;
}

void ModelPrivate::selectNode(const InternalNodePointer &node)
{
    NanotraceHR::Tracer tracer{"model private select node", category()};

    if (selectedNodes().contains(node))
        return;

    FewNodes selectedNodeList(selectedNodes());
    selectedNodeList += node;
    setSelectedNodes(selectedNodeList);
}

void ModelPrivate::deselectNode(const InternalNodePointer &node)
{
    NanotraceHR::Tracer tracer{"model private deselect node", category()};

    FewNodes selectedNodeList(selectedNodes());
    bool isRemoved = selectedNodeList.removeOne(node);

    if (isRemoved)
        setSelectedNodes(selectedNodeList);
}

void ModelPrivate::removePropertyWithoutNotification(InternalProperty *property)
{
    NanotraceHR::Tracer tracer{"model private remove property without notification", category()};

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
    NanotraceHR::Tracer tracer{"model private remove property and related resources", category()};

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
    NanotraceHR::Tracer tracer{"model private remove property", category()};

    removeProperties({property});
}

void ModelPrivate::removeProperties(const QList<InternalProperty *> &properties)
{
    NanotraceHR::Tracer tracer{"model private remove properties", category()};

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
    NanotraceHR::Tracer tracer{"model private set binding property", category()};

    auto [bindingProperty, propertyChange] = node->getBindingProperty(name);

    notifyBindingPropertiesAboutToBeChanged({bindingProperty});
    bindingProperty->setExpression(expression);
    notifyBindingPropertiesChanged({bindingProperty}, propertyChange);
}

void ModelPrivate::setBindingProperties(const ModelResourceSet::SetExpressions &setExpressions)
{
    NanotraceHR::Tracer tracer{"model private set binding properties", category()};

    if (setExpressions.isEmpty())
        return;

    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;

    auto bindingPropertiesWithExpressions = toInternalBindingProperties(setExpressions);

    auto bindingProperties = Utils::transform(bindingPropertiesWithExpressions,
                                              [](const auto &entry) { return std::get<0>(entry); });

    notifyBindingPropertiesAboutToBeChanged(bindingProperties);
    for (const auto &[bindingProperty, expression] : bindingPropertiesWithExpressions)
        bindingProperty->setExpression(expression);
    notifyBindingPropertiesChanged(bindingProperties, propertyChange);
}

void ModelPrivate::setSignalHandlerProperty(const InternalNodePointer &node,
                                            PropertyNameView name,
                                            const QString &source)
{
    NanotraceHR::Tracer tracer{"model private set signal handler property", category()};

    auto [signalHandlerProperty, propertyChange] = node->getSignalHandlerProperty(name);

    signalHandlerProperty->setSource(source);
    notifySignalHandlerPropertiesChanged({signalHandlerProperty}, propertyChange);
}

void ModelPrivate::setSignalDeclarationProperty(const InternalNodePointer &node,
                                                PropertyNameView name,
                                                const QString &signature)
{
    NanotraceHR::Tracer tracer{"model private set signal declaration property", category()};

    auto [signalDeclarationProperty, propertyChange] = node->getSignalDeclarationProperty(name);

    signalDeclarationProperty->setSignature(signature);
    notifySignalDeclarationPropertiesChanged({signalDeclarationProperty}, propertyChange);
}

void ModelPrivate::setVariantProperty(const InternalNodePointer &node,
                                      PropertyNameView name,
                                      const QVariant &value)
{
    NanotraceHR::Tracer tracer{"model private set variant property", category()};

    auto [variantProperty, propertyChange] = node->getVariantProperty(name);

    variantProperty->setValue(value);
    variantProperty->resetDynamicTypeName();
    notifyVariantPropertiesChanged(node, PropertyNameViews({name}), propertyChange);
}

void ModelPrivate::setDynamicVariantProperty(const InternalNodePointer &node,
                                             PropertyNameView name,
                                             const TypeName &dynamicPropertyType,
                                             const QVariant &value)
{
    NanotraceHR::Tracer tracer{"model private set dynamic variant property", category()};

    auto [variantProperty, propertyChange] = node->getVariantProperty(name);

    variantProperty->setDynamicValue(dynamicPropertyType, value);
    notifyVariantPropertiesChanged(node, PropertyNameViews({name}), propertyChange);
}

void ModelPrivate::setDynamicBindingProperty(const InternalNodePointer &node,
                                             PropertyNameView name,
                                             const TypeName &dynamicPropertyType,
                                             const QString &expression)
{
    NanotraceHR::Tracer tracer{"model private set dynamic binding property", category()};

    auto [bindingProperty, propertyChange] = node->getBindingProperty(name);

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
    NanotraceHR::Tracer tracer{"model private reparent node", category()};

    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;

    InternalNodeAbstractPropertyPointer oldParentProperty(childNode->parentProperty());
    InternalNodePointer oldParentNode;
    Utils::SmallString oldParentPropertyName;
    if (oldParentProperty && oldParentProperty->isValid()) {
        oldParentNode = childNode->parentProperty()->propertyOwner();
        oldParentPropertyName = childNode->parentProperty()->name();
    }

    notifyNodeAboutToBeReparent(
        childNode, parentNode, name, oldParentNode, oldParentPropertyName, propertyChange);

    InternalNodeAbstractProperty *newParentProperty = nullptr;
    AbstractView::PropertyChangeFlags newPropertyChange = AbstractView::NoAdditionalChanges;

    if (list)
        std::tie(newParentProperty, newPropertyChange) = parentNode->getNodeListProperty(name);
    else
        std::tie(newParentProperty, newPropertyChange) = parentNode->getNodeProperty(name,
                                                                                     dynamicTypeName);
    propertyChange |= newPropertyChange;

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
    NanotraceHR::Tracer tracer{"model private clear parent", category()};

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

void ModelPrivate::changeRootNodeType(const TypeName &typeName, int majorVersion, int minorVersion)
{
    NanotraceHR::Tracer tracer{"model private change root node type", category()};

    Q_ASSERT(rootNode());

    m_rootInternalNode->traceToken.tick("type name", keyValue("type name", typeName));

    auto [alias, unqualifiedTypeName] = StringUtils::split_last(typeName);

    m_rootInternalNode->typeName = typeName;
    m_rootInternalNode->unqualifiedTypeName = unqualifiedTypeName;
    m_rootInternalNode->majorVersion = majorVersion;
    m_rootInternalNode->minorVersion = minorVersion;
    setTypeId(m_rootInternalNode.get(), alias, unqualifiedTypeName);
    notifyRootNodeTypeChanged(QString::fromUtf8(typeName), majorVersion, minorVersion);
}

void ModelPrivate::setScriptFunctions(const InternalNodePointer &node,
                                      const QStringList &scriptFunctionList)
{
    m_rootInternalNode->traceToken.tick("script function");
    NanotraceHR::Tracer tracer{"model private set script functions", category()};

    node->scriptFunctions = scriptFunctionList;

    notifyScriptFunctionsChanged(node, scriptFunctionList);
}

void ModelPrivate::setNodeSource(const InternalNodePointer &node, const QString &nodeSource)
{
    m_rootInternalNode->traceToken.tick("node source");
    NanotraceHR::Tracer tracer{"model private set node source", category()};

    node->nodeSource = nodeSource;
    notifyNodeSourceChanged(node, nodeSource);
}

void ModelPrivate::changeNodeOrder(const InternalNodePointer &parentNode,
                                   PropertyNameView listPropertyName,
                                   int from,
                                   int to)
{
    NanotraceHR::Tracer tracer{"model private change node order", category()};

    auto nodeList = parentNode->nodeListProperty(listPropertyName);
    Q_ASSERT(nodeList);
    nodeList->slide(from, to);

    const InternalNodePointer internalNode = nodeList->nodeList().at(to);
    notifyNodeOrderChanged(nodeList, internalNode, from);
}

void ModelPrivate::setRewriterView(RewriterView *rewriterView)
{
    NanotraceHR::Tracer tracer{"model private set rewriter view", category()};

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
    NanotraceHR::Tracer tracer{"model private rewriter view", category()};

    return m_rewriterView.data();
}

void ModelPrivate::setNodeInstanceView(AbstractView *nodeInstanceView)
{
    NanotraceHR::Tracer tracer{"model private set node instance view", category()};

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
    NanotraceHR::Tracer tracer{"model private node instance view", category()};

    return m_nodeInstanceView.data();
}

InternalNodePointer ModelPrivate::currentTimelineNode() const
{
    NanotraceHR::Tracer tracer{"model private current timeline node", category()};

    return m_currentTimelineNode;
}

InternalNodePointer ModelPrivate::nodeForId(QStringView id) const
{
    NanotraceHR::Tracer tracer{"model private node for id", category()};

    return m_idNodeHash.value(id.toString());
}

bool ModelPrivate::hasId(QStringView id) const
{
    NanotraceHR::Tracer tracer{"model private has id", category()};

    return m_idNodeHash.contains(id.toString());
}

InternalNodePointer ModelPrivate::nodeForInternalId(qint32 internalId) const
{
    NanotraceHR::Tracer tracer{"model private node for internal id", category()};

    return m_internalIdNodeHash.value(internalId);
}

bool ModelPrivate::hasNodeForInternalId(qint32 internalId) const
{
    NanotraceHR::Tracer tracer{"model private has node for internal id", category()};

    return m_internalIdNodeHash.contains(internalId);
}

ModelPrivate::ManyNodes ModelPrivate::allNodesOrdered() const
{
    NanotraceHR::Tracer tracer{"model private all nodes ordered", category()};

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
    NanotraceHR::Tracer tracer{"model private all nodes unordered", category()};

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
             std::unique_ptr<ModelResourceManagementInterface> resourceManagement,
             SL sl)
{
    NanotraceHR::Tracer tracer{"model constructed with metainfoproxymodel",
                               category(),
                               keyValue("caller location", sl)};

    d = std::make_unique<Internal::ModelPrivate>(this,
                                                 projectStorageDependencies,
                                                 typeName,
                                                 major,
                                                 minor,
                                                 metaInfoProxyModel,
                                                 std::move(resourceManagement));
}

Model::Model(ProjectStorageDependencies projectStorageDependencies,
             Utils::SmallStringView typeName,
             Imports imports,
             const QUrl &fileUrl,
             std::unique_ptr<ModelResourceManagementInterface> resourceManagement,
             SL sl)
{
    NanotraceHR::Tracer tracer{"model constructed with file url",
                               category(),
                               keyValue("caller location", sl)};

    d = std::make_unique<Internal::ModelPrivate>(this,
                                                 projectStorageDependencies,
                                                 typeName,
                                                 std::move(imports),
                                                 fileUrl,
                                                 std::move(resourceManagement));
}

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
                                std::unique_ptr<ModelResourceManagementInterface> resourceManagement,
                                SL sl)
{
    NanotraceHR::Tracer tracer{"model create model", category(), keyValue("caller location", sl)};

    return Model::create({*d->projectStorage,
                          *d->pathCache,
                          *d->modulesStorage,
                          *d->projectStorageTriggerUpdate},
                         typeName,
                         imports(),
                         fileUrl(),
                         std::move(resourceManagement),
                         sl);
}

Model::~Model() = default;

const Imports &Model::imports() const
{
    return d->imports();
}

ModuleIds Model::moduleIds(SL sl) const
{
    NanotraceHR::Tracer tracer{"model module ids", category(), keyValue("caller location", sl)};

    return {};
}

Storage::Info::ExportedTypeName Model::exportedTypeNameForMetaInfo(const NodeMetaInfo &metaInfo,
                                                                   SL sl) const
{
    NanotraceHR::Tracer tracer{"model exported type name for meta info",
                               category(),
                               keyValue("caller location", sl)};

    auto exportedTypeNames = metaInfo.exportedTypeNamesForSourceId(d->m_sourceId);

    if (exportedTypeNames.size())
        return exportedTypeNames.front();

    return {};
}

#ifdef QDS_USE_PROJECTSTORAGE

namespace {

QmlDesigner::Imports createPossibleFileImports(const Utils::FilePath &path)
{
    auto folder = path.parentDir();
    QmlDesigner::Imports imports;

    /* Creates imports for all sub folder that contain a qml file. */
    folder.iterateDirectory(
        [&](const Utils::FilePath &item) {
            bool append = false;

            item.iterateDirectory(
                [&](const Utils::FilePath &) {
                    append = true;
                    return Utils::IterationPolicy::Stop;
                },
                {{"*.qml"}, QDir::Files});
            if (append)
                imports.append(QmlDesigner::Import::createFileImport(item.fileName()));
            return Utils::IterationPolicy::Continue;
        },
        {{}, QDir::Dirs | QDir::NoDotAndDotDot});

    return imports;
}

QmlDesigner::Imports createQt6ModulesForProjectStorage()
{
    return {
        QmlDesigner::Import::createLibraryImport("QtQuick"),
        QmlDesigner::Import::createLibraryImport("QtQuick.Controls"),
        QmlDesigner::Import::createLibraryImport("QtQuick.Window"),
        QmlDesigner::Import::createLibraryImport("QtQuick.VectorImage"),
        QmlDesigner::Import::createLibraryImport("QtQuick.Layouts"),
        QmlDesigner::Import::createLibraryImport("QtQuick.Timeline"),
        QmlDesigner::Import::createLibraryImport("QtCharts"),
        QmlDesigner::Import::createLibraryImport("QtGraphs"),
        QmlDesigner::Import::createLibraryImport("QtInsightTracker"),
        QmlDesigner::Import::createLibraryImport("QtMultimedia"),

        QmlDesigner::Import::createLibraryImport("QtQuick.VirtualKeyboard"),
        QmlDesigner::Import::createLibraryImport("QtQuick.VirtualKeyboard.Components"),
        QmlDesigner::Import::createLibraryImport("QtQuick.VirtualKeyboard.Layouts"),
        QmlDesigner::Import::createLibraryImport("QtQuick.VirtualKeyboard.Settings"),
        QmlDesigner::Import::createLibraryImport("QtQuick.VirtualKeyboard.Styles"),

        QmlDesigner::Import::createLibraryImport("QtQuick3D"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.AssetUtils"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.Effects"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.Helpers"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.Particles3D"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.Physics"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.Physics.Helpers"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.SpatialAudio"),
        QmlDesigner::Import::createLibraryImport("QtQuick3D.Xr"),

        QmlDesigner::Import::createLibraryImport("QtQuickUltralite.Extras"),
        QmlDesigner::Import::createLibraryImport("QtQuickUltralite.Layers"),
        QmlDesigner::Import::createLibraryImport("QtQuickUltralite.Studio.Components"),

        QmlDesigner::Import::createLibraryImport("QtQuick.Studio.Components"),
        QmlDesigner::Import::createLibraryImport("QtQuick.Studio.DesignEffects"),
        QmlDesigner::Import::createLibraryImport("QtQuick.Studio.LogicHelper"),

        QmlDesigner::Import::createLibraryImport("Qt.SafeRenderer"),

        QmlDesigner::Import::createLibraryImport("SimulinkConnector")};
};

} //namespace

#endif //QDS_USE_PROJECTSTORAGE

Imports Model::possibleImports([[maybe_unused]] SL sl) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    NanotraceHR::Tracer tracer{"model possible imports", category(), keyValue("caller location", sl)};

    static auto qt6Imports = createQt6ModulesForProjectStorage();
    auto imports = createPossibleFileImports(Utils::FilePath::fromUrl(fileUrl()));
    imports.append(qt6Imports);
    std::ranges::sort(imports);

    return imports;
#else
    return d->m_possibleImportList;
#endif
}

Imports Model::usedImports() const
{
#ifdef QDS_USE_PROJECTSTORAGE
    return imports();
#else
    return d->m_usedImportList;
#endif
}

void Model::changeImports(Imports importsToBeAdded, Imports importsToBeRemoved, SL sl)
{
    NanotraceHR::Tracer tracer{"model change imports", category(), keyValue("caller location", sl)};

    d->changeImports(std::move(importsToBeAdded), std::move(importsToBeRemoved));
}

void Model::setImports(Imports imports, SL sl)
{
    NanotraceHR::Tracer tracer{"model set imports", category(), keyValue("caller location", sl)};

    d->setImports(std::move(imports));
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

bool Model::hasImport(const Import &import, bool ignoreAlias, bool allowHigherVersion, SL sl) const
{
    NanotraceHR::Tracer tracer{"model has import", category(), keyValue("caller location", sl)};

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

bool Model::hasImport(const QString &importUrl, SL sl) const
{
    NanotraceHR::Tracer tracer{"model has import with import url",
                               category(),
                               keyValue("caller location", sl)};

    return std::ranges::any_of(imports(), is_equal_to(importUrl), &Import::url);
}

QString Model::generateNewId(const QString &prefixName, const QString &fallbackPrefix, SL sl) const
{
    NanotraceHR::Tracer tracer{"model generate id", category(), keyValue("caller location", sl)};

    return UniqueName::generateId(prefixName, fallbackPrefix, [&](const QString &id) {
        // Properties of the root node are not allowed for ids, because they are available in the
        // complete context without qualification.
        return hasId(id) || d->rootNode()->property(id.toUtf8());
    });
}

void Model::startDrag(std::unique_ptr<QMimeData> mimeData, const QPixmap &icon, QWidget *dragSource, SL sl)
{
    NanotraceHR::Tracer tracer{"model start drag", category(), keyValue("caller location", sl)};

    d->notifyDragStarted(mimeData.get());

    d->drag = Utils::makeUniqueObjectPtr<QDrag>(dragSource);
    d->drag->setPixmap(icon);
    d->drag->setMimeData(mimeData.release());
    if (d->drag->exec() == Qt::IgnoreAction)
        endDrag();
}

void Model::endDrag(SL sl)
{
    NanotraceHR::Tracer tracer{"model end drag", category(), keyValue("caller location", sl)};

    if (!d->drag)
        return;

    d->notifyDragEnded();
    d->drag.reset();
}

void Model::setCurrentStateNode(const ModelNode &node, SL sl)
{
    NanotraceHR::Tracer tracer{"model set current state node",
                               category(),
                               keyValue("caller location", sl)};

    Internal::WriteLocker locker(this);
    d->m_currentStateNode = node.internalNode();
    d->notifyCurrentStateChanged(node);
}

ModelNode Model::currentStateNode(AbstractView *view, SL sl)
{
    NanotraceHR::Tracer tracer{"model current state node", category(), keyValue("caller location", sl)};

    return ModelNode(d->currentStateNode(), this, view);
}

void Model::setCurrentTimelineNode(const ModelNode &timeline, SL sl)
{
    NanotraceHR::Tracer tracer{"model set current timeline node",
                               category(),
                               keyValue("caller location", sl)};

    if (timeline.internalNode() == d->m_currentTimelineNode)
        return;

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

ProjectStorageTriggerUpdateInterface &Model::projectStorageTriggerUpdate() const
{
    return *d->projectStorageTriggerUpdate;
}

ProjectStorageDependencies Model::projectStorageDependencies() const
{
    return ProjectStorageDependencies{*d->projectStorage,
                                      *d->pathCache,
                                      *d->modulesStorage,
                                      *d->projectStorageTriggerUpdate};
}

void Model::emitInstancePropertyChange(AbstractView *view,
                                       Utils::span<const QPair<ModelNode, PropertyName>> properties,
                                       SL sl)
{
    NanotraceHR::Tracer tracer{"model emit instance property change",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancePropertyChange(properties);
}

void Model::emitInstanceErrorChange(AbstractView *view, Utils::span<const qint32> instanceIds, SL sl)
{
    NanotraceHR::Tracer tracer{"model emite instance error change",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstanceErrorChange(instanceIds);
}

void Model::emitInstancesCompleted(AbstractView *view, Utils::span<const ModelNode> nodes, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit instances completed",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesCompleted(nodes);
}

void Model::emitInstanceInformationsChange(
    AbstractView *view, const QMultiHash<ModelNode, InformationName> &informationChangeHash, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit instance informations change",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesInformationsChange(informationChangeHash);
}

void Model::emitInstancesRenderImageChanged(AbstractView *view, Utils::span<const ModelNode> nodes, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit isntances render image changed",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesRenderImageChanged(nodes);
}

void Model::emitInstancesPreviewImageChanged(AbstractView *view,
                                             Utils::span<const ModelNode> nodes,
                                             SL sl)
{
    NanotraceHR::Tracer tracer{"model emit instances preview image changed",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesPreviewImageChanged(nodes);
}

void Model::emitInstancesChildrenChanged(AbstractView *view, Utils::span<const ModelNode> nodes, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit instances children changed",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstancesChildrenChanged(nodes);
}

void Model::emitInstanceToken(
    AbstractView *view, const QString &token, int number, const QVector<ModelNode> &nodeVector, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit instance token",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyInstanceToken(token, number, nodeVector);
}

void Model::emitRenderImage3DChanged(AbstractView *view, const QImage &image, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit render image 3d changed",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyRenderImage3DChanged(image);
}

void Model::emitUpdateActiveScene3D(AbstractView *view, const QVariantMap &sceneState, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit update active scene 3d",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyUpdateActiveScene3D(sceneState);
}

void Model::emitModelNodelPreviewPixmapChanged(AbstractView *view,
                                               const ModelNode &node,
                                               const QPixmap &pixmap,
                                               const QByteArray &requestId,
                                               SL sl)
{
    NanotraceHR::Tracer tracer{"model emit model node preview pixmap changed",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyModelNodePreviewPixmapChanged(node, pixmap, requestId);
}

void Model::emitImport3DSupportChanged(AbstractView *view, const QVariantMap &supportMap, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit import 3d support changed",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyImport3DSupportChanged(supportMap);
}

void Model::emitNodeAtPosResult(AbstractView *view,
                                const ModelNode &modelNode,
                                const QVector3D &pos3d,
                                SL sl)
{
    NanotraceHR::Tracer tracer{"model emit node at pos result",
                               category(),
                               keyValue("caller location", sl)};

    if (d->nodeInstanceView() == view) // never remove check
        d->notifyNodeAtPosResult(modelNode, pos3d);
}

void Model::emitView3DAction(View3DActionType type, const QVariant &value, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit view 3d action",
                               category(),
                               keyValue("caller location", sl)};

    d->notifyView3DAction(type, value);
}

void Model::emitDocumentMessage(const QString &error, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit document message with error",
                               category(),
                               keyValue("caller location", sl)};

    emitDocumentMessage({DocumentMessage(error)});
}

void Model::emitDocumentMessage(const QList<DocumentMessage> &errors,
                                const QList<DocumentMessage> &warnings,
                                SL sl)
{
    NanotraceHR::Tracer tracer{"model emit document message with errors and warnings",
                               category(),
                               keyValue("caller location", sl)};

    d->setDocumentMessages(errors, warnings);
}

void Model::emitCustomNotification(AbstractView *view,
                                   const QString &identifier,
                                   Utils::span<const ModelNode> nodes,
                                   const QList<QVariant> &data,
                                   SL sl)
{
    NanotraceHR::Tracer tracer{"model emit custom notification",
                               category(),
                               keyValue("caller location", sl)};

    d->notifyCustomNotification(view, identifier, nodes, data);
}

void Model::sendCustomNotificationTo(AbstractView *to, const CustomNotificationPackage &package, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit custom notification to",
                               category(),
                               keyValue("caller location", sl)};

    d->notifyCustomNotificationTo(to, package);
}

void Model::sendCustomNotificationToNodeInstanceView(const CustomNotificationPackage &package, SL sl)
{
    NanotraceHR::Tracer tracer{"model emit custom notification to node instance view",
                               category(),
                               keyValue("caller location", sl)};

    if (auto view = d->nodeInstanceView())
        d->notifyCustomNotificationTo(view, package);
}

void Model::emitRewriterBeginTransaction(SL sl)
{
    NanotraceHR::Tracer tracer{"model emit rewriter begin transaction",
                               category(),
                               keyValue("caller location", sl)};

    d->notifyRewriterBeginTransaction();
}

void Model::emitRewriterEndTransaction(SL sl)
{
    NanotraceHR::Tracer tracer{"model emit rewriter end transaction",
                               category(),
                               keyValue("caller location", sl)};

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

bool Model::isImportPossible(const Import &import, bool ignoreAlias, bool allowHigherVersion, SL sl) const
{
    NanotraceHR::Tracer tracer{"model is import possible", category(), keyValue("caller location", sl)};

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

QStringList Model::importPaths(SL sl) const
{
    NanotraceHR::Tracer tracer{"model import paths", category(), keyValue("caller location", sl)};

    if (rewriterView())
        return rewriterView()->importDirectories();

    QString documentDirectoryPath = QFileInfo(fileUrl().toLocalFile()).absolutePath();
    if (!documentDirectoryPath.isEmpty())
        return {documentDirectoryPath};

    return {};
}

Import Model::highestPossibleImport(const QString &importPath, SL sl)
{
    NanotraceHR::Tracer tracer{"model highest possible import",
                               category(),
                               keyValue("caller location", sl)};

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

void Model::setRewriterView(RewriterView *rewriterView, SL sl)
{
    NanotraceHR::Tracer tracer{"model set rewriter view", category(), keyValue("caller location", sl)};

    d->setRewriterView(rewriterView);
}

const AbstractView *Model::nodeInstanceView() const
{
    return d->nodeInstanceView();
}

void Model::setNodeInstanceView(AbstractView *nodeInstanceView, SL sl)
{
    NanotraceHR::Tracer tracer{"model set node instance view",
                               category(),
                               keyValue("caller location", sl)};

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
                                const QList<DocumentMessage> &warnings,
                                SL sl)
{
    NanotraceHR::Tracer tracer{"model set document messages",
                               category(),
                               keyValue("caller location", sl)};

    d->setDocumentMessages(errors, warnings);
}

/*!
 * \brief Returns list of selected nodes for a view
 */
QList<ModelNode> Model::selectedNodes(AbstractView *view, SL sl) const
{
    NanotraceHR::Tracer tracer{"model selected nodes", category(), keyValue("caller location", sl)};

    return d->toModelNodeList(d->selectedNodes(), view);
}

void Model::setSelectedModelNodes(Utils::span<const ModelNode> selectedNodes, SL sl)
{
    NanotraceHR::Tracer tracer{"model set selected model nodes",
                               category(),
                               keyValue("caller location", sl)};

    QList<ModelNode> unlockedNodes;

    for (const auto &modelNode : selectedNodes) {
        if (!ModelUtils::isThisOrAncestorLocked(modelNode))
            unlockedNodes.push_back(modelNode);
    }

    d->setSelectedNodes(toInternalNodeList<Internal::InternalNode::FewNodes>(unlockedNodes));
}

void Model::clearMetaInfoCache(SL sl)
{
    NanotraceHR::Tracer tracer{"model clear meta info cache",
                               category(),
                               keyValue("caller location", sl)};

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
void Model::setFileUrl(const QUrl &url, SL sl)
{
    NanotraceHR::Tracer tracer{"model set file url", category(), keyValue("caller location", sl)};

    Internal::WriteLocker locker(d.get());
    d->setFileUrl(url);
}

bool Model::hasNodeMetaInfo(const TypeName &typeName, int majorVersion, int minorVersion, SL sl) const
{
    NanotraceHR::Tracer tracer{"model has node meta info", category(), keyValue("caller location", sl)};

    return metaInfo(typeName, majorVersion, minorVersion).isValid();
}

#ifndef QDS_USE_PROJECTSTORAGE
MetaInfo Model::metaInfo()
{
    return d->metaInfo();
}

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

NodeMetaInfo Model::floatMetaInfo() const
{
#ifdef QDS_USE_PROJECTSTORAGE
    using namespace Storage::Info;
    using Storage::ModuleKind;

    return {d->projectStorage->commonTypeId<QML, FloatType, ModuleKind::CppLibrary>(),
            d->projectStorage};
#else
    return {};
#endif
}

template<Storage::Info::StaticString moduleName, Storage::Info::StaticString typeName, Storage::ModuleKind moduleKind>
NodeMetaInfo Model::createNodeMetaInfo() const
{
    auto typeId = d->projectStorage->commonTypeCache().typeId<moduleName, typeName, moduleKind>();

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

NodeMetaInfo Model::qtQuickShapesShapeMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Shapes, Shape>();
    } else {
        return metaInfo("QtQuick.Shapes.Shape");
    }
}

NodeMetaInfo Model::qtQuickGradientMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Gradient>();
    } else {
        return metaInfo("QtQuick.Gradient");
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

NodeMetaInfo Model::qtQuickTextEditMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, TextEdit>();
    } else {
        return metaInfo("QtQuick.TextEdit");
    }
}

NodeMetaInfo Model::qtQuickTemplatesLabelMetaInfo() const
{
#ifdef QDS_USE_PROJECTSTORAGE
    using namespace Storage::Info;
    return createNodeMetaInfo<QtQuick_Templates, Label>();
#else
    return metaInfo("QtQuick.Controls.Label");
#endif
}

NodeMetaInfo Model::qtQuickTemplatesTextAreaMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Templates, TextArea>();
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

NodeMetaInfo Model::qtQuick3DObject3DMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Object3D>();
    } else {
        return metaInfo("QtQuick3D.Object3D");
    }
}

NodeMetaInfo Model::qtQuick3DCameraMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Camera>();
    } else {
        return metaInfo("QtQuick3D.Camera");
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

NodeMetaInfo Model::qtQuick3DView3DMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, View3D>();
    } else {
        return metaInfo("QtQuick3D.View3D");
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

NodeMetaInfo Model::qtQuick3DLightMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Light>();
    } else {
        return metaInfo("QtQuick3D.Light");
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

NodeMetaInfo Model::qtQuickWindowMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Window>();
    } else {
        return metaInfo("QtQuick.Window.Window");
    }
}

NodeMetaInfo Model::qtQuickDialogsAbstractDialogMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Dialogs, QQuickAbstractDialog, Storage::ModuleKind::CppLibrary>();
    } else {
        return metaInfo("QtQuick.Dialogs.Dialog");
    }
}

NodeMetaInfo Model::qtQuickTemplatesPopupMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Templates, Popup>();
    } else {
        return metaInfo("QtQuick.Controls.Popup");
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

QVarLengthArray<NodeMetaInfo, 256> Model::metaInfosForModule(Module module, SL sl) const
{
    NanotraceHR::Tracer tracer{"model meta infos for module",
                               category(),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;

    return Utils::transform<QVarLengthArray<NodeMetaInfo, 256>>(d->projectStorage->typeIds(module.id()),
                                                                NodeMetaInfo::bind(d->projectStorage));
}

SmallNodeMetaInfos<256> Model::singletonMetaInfos(SL sl) const
{
    NanotraceHR::Tracer tracer{"model singleton meta infos",
                               category(),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;

    return Utils::transform<QVarLengthArray<NodeMetaInfo, 256>>(d->projectStorage->singletonTypeIds(
                                                                    d->m_sourceId),
                                                                NodeMetaInfo::bind(d->projectStorage));
}

#endif

QList<ItemLibraryEntry> Model::itemLibraryEntries([[maybe_unused]] SL sl) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    NanotraceHR::Tracer tracer{"model item library entries",
                               category(),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return toItemLibraryEntries(*d->pathCache, d->projectStorage->itemLibraryEntries(d->m_sourceId));
#else
    return d->metaInfo().itemLibraryInfo()->entries();
#endif
}

QList<ItemLibraryEntry> Model::directoryImportsItemLibraryEntries([[maybe_unused]] SL sl) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    NanotraceHR::Tracer tracer{"model directory imports item library entries",
                               category(),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return toItemLibraryEntriesFromDirectory(*d->pathCache,
                                             d->projectStorage->directoryImportsItemLibraryEntries(
                                                 d->m_sourceId),
                                             d->m_sourceId.directoryPathId(),
                                             d->m_localPath);
#else
    return {};
#endif
}

QList<ItemLibraryEntry> Model::allItemLibraryEntries([[maybe_unused]] SL sl) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    NanotraceHR::Tracer tracer{"model all item library entries",
                               category(),
                               keyValue("caller location", sl)};

    using namespace Storage::Info;
    return toItemLibraryEntries(*d->pathCache, d->projectStorage->allItemLibraryEntries());
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

NodeMetaInfo Model::metaInfo(Utils::SmallStringView typeName,
                             [[maybe_unused]] int majorVersion,
                             [[maybe_unused]] int minorVersion,
                             [[maybe_unused]] SL sl) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    NanotraceHR::Tracer tracer{"model meta info with type name",
                               category(),
                               keyValue("caller location", sl)};

    return NodeMetaInfo(d->projectStorage->exportedTypeName(d->importedTypeNameId(typeName)).typeId,
                        d->projectStorage);
#else
    return NodeMetaInfo(metaInfoProxyModel(), typeName.toByteArray(), majorVersion, minorVersion);
#endif
}

NodeMetaInfo Model::metaInfo([[maybe_unused]] Module module,
                             [[maybe_unused]] Utils::SmallStringView typeName,
                             [[maybe_unused]] Storage::Version version,
                             [[maybe_unused]] SL sl) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    NanotraceHR::Tracer tracer{"model meta info with module",
                               category(),
                               keyValue("caller location", sl)};

    return NodeMetaInfo(d->projectStorage->typeId(module.id(), typeName, version), d->projectStorage);
#else
    return {};
#endif
}

Module Model::module(Utils::SmallStringView moduleName, Storage::ModuleKind moduleKind, SL sl) const
{
    if constexpr (useProjectStorage()) {
        NanotraceHR::Tracer tracer{"model get module", category(), keyValue("caller location", sl)};

        return Module(d->modulesStorage->moduleId(moduleName, moduleKind), d->projectStorage);
    }

    return {};
}

SmallModuleIds<128> Model::moduleIdsStartsWith(Utils::SmallStringView startsWith,
                                               Storage::ModuleKind kind,
                                               SL sl) const
{
    if constexpr (useProjectStorage()) {
        NanotraceHR::Tracer tracer{"model module ids starts with",
                                   category(),
                                   keyValue("caller location", sl)};

        return d->modulesStorage->moduleIdsStartsWith(startsWith, kind);
    }

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
void Model::attachView(AbstractView *view, SL sl)
{
    NanotraceHR::Tracer tracer{"model attach view",
                               category(),
                               keyValue("name", view->name()),
                               keyValue("caller location", sl)};

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
void Model::detachView(AbstractView *view, ViewNotification emitDetachNotify, SL sl)
{
    NanotraceHR::Tracer tracer{"model detach view",
                               category(),
                               keyValue("name", view->name()),
                               keyValue("caller location", sl)};

    //    Internal::WriteLocker locker(d);
    bool emitNotify = (emitDetachNotify == NotifyView);

    if (view->kind() != AbstractView::Kind::Other)
        return;

    d->detachView(view, emitNotify);
}

QList<ModelNode> Model::allModelNodesUnordered(SL sl)
{
    NanotraceHR::Tracer tracer{"model all model nodes unordered",
                               category(),
                               keyValue("caller location", sl)};

    return toModelNodeList(d->allNodesUnordered(), this);
}

ModelNode Model::rootModelNode(SL sl)
{
    NanotraceHR::Tracer tracer{"model root model node", category(), keyValue("caller location", sl)};

    return ModelNode{d->rootNode(), this, nullptr};
}

ModelNode Model::modelNodeForId(const QString &id, SL sl)
{
    NanotraceHR::Tracer tracer{"model model node for id", category(), keyValue("caller location", sl)};

    return ModelNode(d->nodeForId(id), this, nullptr);
}

QHash<QStringView, ModelNode> Model::idModelNodeDict(SL sl)
{
    NanotraceHR::Tracer tracer{"model id model node dict", category(), keyValue("caller location", sl)};

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

ModelNode Model::createModelNode(const TypeName &typeName, [[maybe_unused]] SL sl)
{
#ifdef QDS_USE_PROJECTSTORAGE
    NanotraceHR::Tracer tracer{"model create model node", category(), keyValue("caller location", sl)};

    return createNode(this, d.get(), typeName, -1, -1);
#else
    const NodeMetaInfo m = metaInfo(typeName);
    return createNode(this, d.get(), typeName, m.majorVersion(), m.minorVersion());
#endif
}

void Model::changeRootNodeType(const TypeName &type, SL sl)
{
    NanotraceHR::Tracer tracer{"model change root node type",
                               category(),
                               keyValue("caller location", sl)};

    Internal::WriteLocker locker(this);

    d->changeRootNodeType(type, -1, -1);
}

void Model::removeModelNodes(ModelNodes nodes, BypassModelResourceManagement bypass, SL sl)
{
    NanotraceHR::Tracer tracer{"model remove model nodes", category(), keyValue("caller location", sl)};

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

void Model::removeProperties(AbstractProperties properties, BypassModelResourceManagement bypass, SL sl)
{
    NanotraceHR::Tracer tracer{"model remove properties", category(), keyValue("caller location", sl)};

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
