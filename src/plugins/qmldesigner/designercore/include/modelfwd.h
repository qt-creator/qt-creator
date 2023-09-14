// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectstorage/projectstoragefwd.h"
#include "qmldesignercorelib_exports.h"

#include <utils/smallstringview.h>
#include <utils/span.h>

namespace QmlDesigner {
using PropertyName = QByteArray;
using PropertyNameView = QByteArrayView;
using PropertyNameList = QList<PropertyName>;

template<std::size_t Size, const char Text[Size]>
class ByteArrayViewLiteral : public QByteArrayView
{
public:
    constexpr ByteArrayViewLiteral() noexcept
        : QByteArrayView{Text, Size - 1}
    {}

    const QByteArray &toByteArray() const noexcept
    {
        static auto b = QByteArray::fromRawData(Text, Size - 1);

        return b;
    }

    operator const QByteArray &() const noexcept { return toByteArray(); }
};

#define MakeCppPropertyViewLiteral(name, text) \
    constexpr char name##InternalString[] = text; \
    constexpr ByteArrayViewLiteral<sizeof(name##InternalString), name##InternalString> name{}; \
    /* */

#define MakeHeaderPropertyViewLiteral(name, text) \
    inline constexpr char name##InternalString[] = text; \
    inline constexpr ByteArrayViewLiteral<sizeof(name##InternalString), name##InternalString> name{}; \
    /* */

#define MakeFunctionPropertyViewLiteral(name, text) \
    static constexpr char name##InternalString[] = text; \
    static constexpr ByteArrayViewLiteral<sizeof(name##InternalString), name##InternalString> name{}; \
    /* */

using TypeName = QByteArray;
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

#ifdef QDS_MODEL_USE_PROJECTSTORAGEINTERFACE
using ProjectStorageType = ProjectStorageInterface;
using PathCacheType = SourcePathCacheInterface;
#else
using ProjectStorageType = ProjectStorage<Sqlite::Database>;
using PathCacheType = SourcePathCache<ProjectStorageType, NonLockingMutex>;
#endif

struct ProjectStorageDependencies
{
    ProjectStorageType &storage;
    PathCacheType &cache;
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
