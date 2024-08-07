// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageerrornotifierinterface.h"

#include <modelfwd.h>

namespace QmlDesigner {

class ProjectStorageErrorNotifier final : public ProjectStorageErrorNotifierInterface
{
public:
    ProjectStorageErrorNotifier(PathCacheType &pathCache)
        : m_pathCache{pathCache}
    {}

    void typeNameCannotBeResolved(Utils::SmallStringView typeName, SourceId sourceId) override;
    void missingDefaultProperty(Utils::SmallStringView typeName,
                                Utils::SmallStringView propertyName,
                                SourceId sourceId) override;
    void propertyNameDoesNotExists(Utils::SmallStringView propertyName, SourceId sourceId) override;

private:
    PathCacheType &m_pathCache;
};

} // namespace QmlDesigner
