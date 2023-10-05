// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelutils.h"

#include <abstractview.h>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/sourcepathcache.h>

#include <utils/expected.h>
#include <utils/ranges.h>

#include <algorithm>

namespace QmlDesigner::ModelUtils {

namespace {

enum class ImportError { EmptyImportName, HasAlreadyImport, NoModule };

::Utils::expected<Import, ImportError> findImport(const QString &importName,
                                                  const std::function<bool(const Import &)> &predicate,
                                                  const Imports &imports,
                                                  const Imports &modules)
{
    if (importName.isEmpty())
        return ::Utils::make_unexpected(ImportError::EmptyImportName);

    auto hasName = [&](const auto &import) {
        return import.url() == importName || import.file() == importName;
    };

    bool hasImport = std::any_of(imports.begin(), imports.end(), hasName);

    if (hasImport)
        return ::Utils::make_unexpected(ImportError::HasAlreadyImport);

    auto foundModule = std::find_if(modules.begin(), modules.end(), [&](const Import &import) {
        return hasName(import) && predicate(import);
    });

    if (foundModule == modules.end())
        return ::Utils::make_unexpected(ImportError::NoModule);

    return *foundModule;
}

} // namespace

bool addImportWithCheck(const QString &importName,
                        const std::function<bool(const Import &)> &predicate,
                        Model *model)
{
    return addImportsWithCheck({importName}, predicate, model);
}

bool addImportWithCheck(const QString &importName, Model *model)
{
    return addImportWithCheck(
        importName, [](const Import &) { return true; }, model);
}

bool addImportsWithCheck(const QStringList &importNames, Model *model)
{
    return addImportsWithCheck(
        importNames, [](const Import &) { return true; }, model);
}

bool addImportsWithCheck(const QStringList &importNames,
                         const std::function<bool(const Import &)> &predicate,
                         Model *model)
{
    const Imports &imports = model->imports();
    const Imports &modules = model->possibleImports();

    Imports importsToAdd;
    importsToAdd.reserve(importNames.size());

    for (const QString &importName : importNames) {
        auto import = findImport(importName, predicate, imports, modules);

        if (import) {
            importsToAdd.push_back(*import);
        } else {
            if (import.error() == ImportError::NoModule)
                return false;
            else
                continue;
        }
    }

    if (!importsToAdd.isEmpty())
        model->changeImports(std::move(importsToAdd), {});

    return true;
}

PropertyMetaInfo metainfo(const AbstractProperty &property)
{
    return metainfo(property.parentModelNode(), property.name());
}

PropertyMetaInfo metainfo(const ModelNode &node, const PropertyName &propertyName)
{
    return node.metaInfo().property(propertyName);
}

QString componentFilePath(const PathCacheType &pathCache, const NodeMetaInfo &metaInfo)
{
    if constexpr (useProjectStorage()) {
        auto typeSourceId = metaInfo.sourceId();

        if (typeSourceId && metaInfo.isFileComponent()) {
            return pathCache.sourcePath(typeSourceId).toQString();
        }
    } else {
        return metaInfo.componentFileName();
    }

    return {};
}

QString componentFilePath(const ModelNode &node)
{
    if (node) {
        const auto &pathCache = node.model()->pathCache();
        return ModelUtils::componentFilePath(pathCache, node.metaInfo());
    }

    return {};
}

QList<ModelNode> pruneChildren(const QList<ModelNode> &nodes)
{
    QList<ModelNode> forwardNodes;
    QList<ModelNode> backNodes;

    auto pushIfIsNotChild = [](QList<ModelNode> &container, const ModelNode &node) {
        bool hasAncestor = Utils::anyOf(container, [node](const ModelNode &testNode) -> bool {
            return testNode.isAncestorOf(node);
        });
        if (!hasAncestor)
            container.append(node);
    };

    for (const ModelNode &node : nodes | Utils::views::reverse) {
        if (node)
            pushIfIsNotChild(forwardNodes, node);
    }

    for (const ModelNode &node : forwardNodes | Utils::views::reverse)
        pushIfIsNotChild(backNodes, node);

    return backNodes;
}

QList<ModelNode> allModelNodesWithId(AbstractView *view)
{
    QTC_ASSERT(view->isAttached(), return {});
    return Utils::filtered(view->allModelNodes(),
                           [&](const ModelNode &node) { return node.hasId(); });
}

bool isThisOrAncestorLocked(const ModelNode &node)
{
    if (!node.isValid())
        return false;

    if (node.locked())
        return true;

    if (node.isRootNode() || !node.hasParentProperty())
        return false;

    return isThisOrAncestorLocked(node.parentProperty().parentModelNode());
}

/*!
 * \brief The lowest common ancestor node for node1 and node2. If one of the nodes (Node A) is
 * the ancestor of the other node, the return value is Node A and not the parent of Node A.
 * \param node1 First node
 * \param node2 Second node
 * \param depthOfLCA Depth of the return value
 * \param depthOfNode1 Depth of node1. Use this parameter for optimization
 * \param depthOfNode2 Depth of node2. Use this parameter for optimization
 */
namespace {
ModelNode lowestCommonAncestor(const ModelNode &node1,
                               const ModelNode &node2,
                               int &depthOfLCA,
                               const int &depthOfNode1 = -1,
                               const int &depthOfNode2 = -1)
{
    auto depthOfNode = [](const ModelNode &node) -> int {
        int depth = 0;
        ModelNode parentNode = node;
        while (parentNode && !parentNode.isRootNode()) {
            depth++;
            parentNode = parentNode.parentProperty().parentModelNode();
        }

        return depth;
    };

    if (node1 == node2) {
        depthOfLCA = (depthOfNode1 < 0) ? ((depthOfNode2 < 0) ? depthOfNode(node1) : depthOfNode2)
                                        : depthOfNode1;
        return node1;
    }

    if (node1.model() != node2.model()) {
        depthOfLCA = -1;
        return {};
    }

    if (node1.isRootNode()) {
        depthOfLCA = 0;
        return node1;
    }

    if (node2.isRootNode()) {
        depthOfLCA = 0;
        return node2;
    }

    ModelNode nodeLower = node1;
    ModelNode nodeHigher = node2;
    int depthLower = (depthOfNode1 < 0) ? depthOfNode(nodeLower) : depthOfNode1;
    int depthHigher = (depthOfNode2 < 0) ? depthOfNode(nodeHigher) : depthOfNode2;

    if (depthLower > depthHigher) {
        std::swap(depthLower, depthHigher);
        std::swap(nodeLower, nodeHigher);
    }

    int depthDiff = depthHigher - depthLower;
    while (depthDiff--)
        nodeHigher = nodeHigher.parentProperty().parentModelNode();

    while (nodeLower != nodeHigher) {
        nodeLower = nodeLower.parentProperty().parentModelNode();
        nodeHigher = nodeHigher.parentProperty().parentModelNode();
        --depthLower;
    }

    depthOfLCA = depthLower;
    return nodeLower;
}
} // namespace

/*!
 * \brief The lowest common node containing all nodes. If one of the nodes (Node A) is
 * the ancestor of the other nodes, the return value is Node A and not the parent of Node A.
 */
ModelNode lowestCommonAncestor(Utils::span<const ModelNode> nodes)
{
    if (nodes.empty())
        return {};

    ModelNode accumulatedNode = nodes.front();
    int accumulatedNodeDepth = -1;
    for (const ModelNode &node : nodes.subspan(1)) {
        accumulatedNode = lowestCommonAncestor(accumulatedNode,
                                               node,
                                               accumulatedNodeDepth,
                                               accumulatedNodeDepth);
        if (!accumulatedNode)
            return {};
    }

    return accumulatedNode;
}

} // namespace QmlDesigner::ModelUtils
