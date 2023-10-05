// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <import.h>
#include <model.h>

#include <functional>

namespace QmlDesigner {
class PropertyMetaInfo;
}

namespace QmlDesigner::ModelUtils {

QMLDESIGNERCORE_EXPORT bool addImportsWithCheck(const QStringList &importNames,
                                                const std::function<bool(const Import &)> &predicate,
                                                Model *model);
QMLDESIGNERCORE_EXPORT bool addImportsWithCheck(const QStringList &importNames, Model *model);
QMLDESIGNERCORE_EXPORT bool addImportWithCheck(const QString &importName,
                                               const std::function<bool(const Import &)> &predicate,
                                               Model *model);
QMLDESIGNERCORE_EXPORT bool addImportWithCheck(const QString &importName, Model *model);

QMLDESIGNERCORE_EXPORT PropertyMetaInfo metainfo(const AbstractProperty &property);
QMLDESIGNERCORE_EXPORT PropertyMetaInfo metainfo(const ModelNode &node,
                                                 const PropertyName &propertyName);

QMLDESIGNERCORE_EXPORT QString componentFilePath(const PathCacheType &pathCache,
                                                 const NodeMetaInfo &metaInfo);

QMLDESIGNERCORE_EXPORT QString componentFilePath(const ModelNode &node);

QMLDESIGNERCORE_EXPORT QList<ModelNode> pruneChildren(const QList<ModelNode> &nodes);

QMLDESIGNERCORE_EXPORT QList<ModelNode> allModelNodesWithId(AbstractView *view);

QMLDESIGNERCORE_EXPORT bool isThisOrAncestorLocked(const ModelNode &node);
QMLDESIGNERCORE_EXPORT ModelNode lowestCommonAncestor(Utils::span<const ModelNode> nodes);

} // namespace QmlDesigner::ModelUtils
