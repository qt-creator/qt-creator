// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

#include <utils/smallstringview.h>

namespace QmlDesigner {

class ProjectStorageErrorNotifierInterface
{
public:
    ProjectStorageErrorNotifierInterface() = default;
    ProjectStorageErrorNotifierInterface(ProjectStorageErrorNotifierInterface &&) = default;
    ProjectStorageErrorNotifierInterface &operator=(ProjectStorageErrorNotifierInterface &&) = default;
    ProjectStorageErrorNotifierInterface(const ProjectStorageErrorNotifierInterface &) = delete;
    ProjectStorageErrorNotifierInterface &operator=(const ProjectStorageErrorNotifierInterface &) = delete;

    virtual void typeNameCannotBeResolved(Utils::SmallStringView typeName, SourceId sourceId) = 0;
    virtual void missingDefaultProperty(Utils::SmallStringView typeName,
                                        Utils::SmallStringView propertyName,
                                        SourceId sourceId)
        = 0;
    virtual void propertyNameDoesNotExists(Utils::SmallStringView propertyName, SourceId sourceId) = 0;

protected:
    ~ProjectStorageErrorNotifierInterface() = default;
};

} // namespace QmlDesigner
