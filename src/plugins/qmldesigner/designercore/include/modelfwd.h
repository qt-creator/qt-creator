// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectstorage/projectstoragefwd.h"
#include "qmldesignercorelib_exports.h"

#include <utils/smallstringview.h>

namespace QmlDesigner {
using PropertyName = QByteArray;
using PropertyNameList = QList<PropertyName>;
using TypeName = QByteArray;
using PropertyTypeList = QList<PropertyName>;
using IdName = QByteArray;
class Model;
class ModelNode;

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
#else
using ProjectStorageType = ProjectStorage<Sqlite::Database>;
#endif

} // namespace QmlDesigner
