// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectstorage/projectstoragefwd.h"
#include "qmldesignercorelib_exports.h"

#include <utils/smallstringview.h>
#include <utils/span.h>

#include <shared_mutex>

namespace QmlDesigner {
using PropertyName = QByteArray;
using PropertyNameView = QByteArrayView;
using PropertyNameList = QList<PropertyName>;

using TypeName = QByteArray;
using TypeNameView = QByteArrayView;
using PropertyTypeList = QList<PropertyName>;
using IdName = QByteArray;
class Model;
class ModelNode;
class NonLockingMutex;
template<typename ProjectStorage, typename Mutex = NonLockingMutex>
class SourcePathCache;

struct ModelDeleter
{
    QMLDESIGNERCORE_EXPORT void operator()(class Model *model);
};

using ModelPointer = std::unique_ptr<class Model, ModelDeleter>;

constexpr bool useProjectStorage()
{
#ifdef QDS_USE_PROJECTSTORAGE
    return true;
#else
    return false;
#endif
}
class SourcePathStorage;
class ProjectStorageTriggerUpdateInterface;
using PathCache = SourcePathCache<SourcePathStorage, std::shared_mutex>;

#ifdef QDS_MODEL_USE_PROJECTSTORAGEINTERFACE
using ProjectStorageType = ProjectStorageInterface;
class SourcePathCacheInterface;
using PathCacheType = SourcePathCacheInterface;
#else
using ProjectStorageType = ProjectStorage;
using PathCacheType = SourcePathCache<SourcePathStorage, std::shared_mutex>;
#endif
struct ProjectStorageDependencies
{
    ProjectStorageType &storage;
    PathCacheType &cache;
    ProjectStorageTriggerUpdateInterface &triggerUpdate;
};

enum class PropertyType {
    None,
    Variant,
    Node,
    NodeList,
    Binding,
    SignalHandler,
    SignalDeclaration
};

} // namespace QmlDesigner
