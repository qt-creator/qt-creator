// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelutils.h"

#include <abstractview.h>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>
#include <sourcepathstorage/sourcepathcache.h>


#include <utils/expected.h>
#include <utils/ranges.h>

#include <algorithm>
#include <array>

#include <QRegularExpression>

namespace QmlDesigner::ModelUtils {

namespace {

enum class ImportError { EmptyImportName, HasAlreadyImport, NoModule };

template<typename Type, typename... Entries>
constexpr auto toSortedArray(const Entries &...entries)
{
    std::array<Type, sizeof...(entries)> sortedArray = {entries...};

    std::sort(std::begin(sortedArray), std::end(sortedArray));

    return sortedArray;
}

// alias, print, console are not part of qmljs
constexpr auto qmlKeywords = toSortedArray<std::u16string_view>(u"abstract",
                                                                u"alias",
                                                                u"as",
                                                                u"boolean",
                                                                u"break",
                                                                u"byte",
                                                                u"case",
                                                                u"catch",
                                                                u"char",
                                                                u"class",
                                                                u"component",
                                                                u"console",
                                                                u"const",
                                                                u"continue",
                                                                u"debugger",
                                                                u"default",
                                                                u"delete",
                                                                u"do",
                                                                u"double",
                                                                u"else",
                                                                u"enum",
                                                                u"export",
                                                                u"extends",
                                                                u"false",
                                                                u"final",
                                                                u"finally",
                                                                u"float",
                                                                u"for",
                                                                u"from",
                                                                u"function",
                                                                u"get",
                                                                u"goto",
                                                                u"if",
                                                                u"implements",
                                                                u"import",
                                                                u"in",
                                                                u"instanceof",
                                                                u"int",
                                                                u"interface",
                                                                u"let",
                                                                u"long",
                                                                u"native",
                                                                u"new",
                                                                u"null",
                                                                u"of",
                                                                u"on",
                                                                u"package",
                                                                u"pragma",
                                                                u"print",
                                                                u"private",
                                                                u"property",
                                                                u"protected",
                                                                u"public",
                                                                u"readonly",
                                                                u"required",
                                                                u"return",
                                                                u"set",
                                                                u"short",
                                                                u"signal",
                                                                u"static",
                                                                u"super",
                                                                u"switch",
                                                                u"synchronized",
                                                                u"this",
                                                                u"throw",
                                                                u"throws",
                                                                u"transient",
                                                                u"true",
                                                                u"try",
                                                                u"typeof",
                                                                u"var",
                                                                u"void",
                                                                u"volatile",
                                                                u"while",
                                                                u"with",
                                                                u"yield");

constexpr auto qmlDiscouragedIds = toSortedArray<std::u16string_view>(u"action",
                                                                      u"anchors",
                                                                      u"baseState",
                                                                      u"border",
                                                                      u"bottom",
                                                                      u"clip",
                                                                      u"data",
                                                                      u"enabled",
                                                                      u"flow",
                                                                      u"focus",
                                                                      u"font",
                                                                      u"height",
                                                                      u"id",
                                                                      u"item",
                                                                      u"layer",
                                                                      u"left",
                                                                      u"margin",
                                                                      u"opacity",
                                                                      u"padding",
                                                                      u"parent",
                                                                      u"right",
                                                                      u"scale",
                                                                      u"shaderInfo",
                                                                      u"source",
                                                                      u"sprite",
                                                                      u"spriteSequence",
                                                                      u"state",
                                                                      u"text",
                                                                      u"texture",
                                                                      u"time",
                                                                      u"top",
                                                                      u"visible",
                                                                      u"width",
                                                                      u"x",
                                                                      u"y",
                                                                      u"z");

constexpr auto qmlBuiltinTypes = toSortedArray<std::u16string_view>(u"bool",
                                                                    u"color",
                                                                    u"date",
                                                                    u"double",
                                                                    u"enumeration",
                                                                    u"font",
                                                                    u"int",
                                                                    u"list",
                                                                    u"matrix4x4",
                                                                    u"point",
                                                                    u"quaternion",
                                                                    u"real",
                                                                    u"rect",
                                                                    u"size",
                                                                    u"string",
                                                                    u"url",
                                                                    u"var",
                                                                    u"variant",
                                                                    u"vector",
                                                                    u"vector2d",
                                                                    u"vector3d",
                                                                    u"vector4d");

constexpr auto createBannedQmlIds()
{
    std::array<std::u16string_view, qmlKeywords.size() + qmlDiscouragedIds.size() + qmlBuiltinTypes.size()> ids;

    auto idsEnd = std::copy(std::begin(qmlKeywords), std::end(qmlKeywords), ids.begin());
    idsEnd = std::copy(std::begin(qmlDiscouragedIds), std::end(qmlDiscouragedIds), idsEnd);
    std::copy(std::begin(qmlBuiltinTypes), std::end(qmlBuiltinTypes), idsEnd);

    std::sort(ids.begin(), ids.end());

    return ids;
}

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

    auto foundModule = std::ranges::find_if(modules, [&](const Import &import) {
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

PropertyMetaInfo metainfo(const ModelNode &node, PropertyNameView propertyName)
{
    return node.metaInfo().property(propertyName);
}

QString componentFilePath([[maybe_unused]] const PathCacheType &pathCache, const NodeMetaInfo &metaInfo)
{
#ifdef QDS_USE_PROJECTSTORAGE
    auto typeSourceId = metaInfo.sourceId();

    if (typeSourceId && metaInfo.isFileComponent()) {
        return pathCache.sourcePath(typeSourceId).toQString();
    }

    return {};
#else
    return metaInfo.componentFileName();
#endif
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
    return Utils::filtered(view->allModelNodes(), [&](const ModelNode &node) { return node.hasId(); });
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

bool isQmlKeyword(QStringView id)
{
    return std::binary_search(std::begin(qmlKeywords), std::end(qmlKeywords), toStdStringView(id));
}

bool isDiscouragedQmlId(QStringView id)
{
    return std::binary_search(std::begin(qmlDiscouragedIds),
                              std::end(qmlDiscouragedIds),
                              toStdStringView(id));
}

bool isQmlBuiltinType(QStringView id)
{
    return std::binary_search(std::begin(qmlBuiltinTypes),
                              std::end(qmlBuiltinTypes),
                              toStdStringView(id));
}

bool isBannedQmlId(QStringView id)
{
    static constexpr auto invalidIds = createBannedQmlIds();
    return std::binary_search(invalidIds.begin(), invalidIds.end(), toStdStringView(id));
}

bool isValidQmlIdentifier(QStringView id)
{
    static QRegularExpression idExpr(R"(^[a-z_]\w*$)");
    return id.contains(idExpr);
}

QStringList expressionToList(QStringView expression)
{
    QStringView cleanedExp = (expression.startsWith('[') && expression.endsWith(']'))
                                 ? expression.sliced(1, expression.size() - 2)
                                 : expression;
    QList<QStringView> tokens = cleanedExp.split(',');

    QStringList expList;
    expList.reserve(tokens.size());
    for (QStringView token : tokens) {
        token = token.trimmed();
        if (!token.isEmpty())
            expList.append(token.toString());
    }

    return expList;
}

QString listToExpression(const QStringList &stringList)
{
    if (stringList.size() > 1)
        return QString("[" + stringList.join(",") + "]");

    if (stringList.size() == 1)
        return stringList.first();

    return QString();
}

} // namespace QmlDesigner::ModelUtils
