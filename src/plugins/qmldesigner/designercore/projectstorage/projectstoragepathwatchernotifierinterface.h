// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragepathwatchertypes.h"

#include <utils/smallstringvector.h>

namespace QmlDesigner {

class ProjectStoragePathWatcherNotifierInterface
{
public:
    ProjectStoragePathWatcherNotifierInterface() = default;
    ProjectStoragePathWatcherNotifierInterface(const ProjectStoragePathWatcherNotifierInterface &) = delete;
    ProjectStoragePathWatcherNotifierInterface &operator=(const ProjectStoragePathWatcherNotifierInterface &) = delete;

    virtual void pathsWithIdsChanged(const std::vector<IdPaths> &idPaths) = 0;
    virtual void pathsChanged(const SourceIds &filePathIds) = 0;

protected:
    ~ProjectStoragePathWatcherNotifierInterface() = default;
};

} // namespace QmlDesigner
