// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragepathwatchertypes.h"

#include <utils/smallstringvector.h>

namespace QmlDesigner {

class ProjectStoragePathWatcherNotifierInterface;

class ProjectStoragePathWatcherInterface
{
public:
    ProjectStoragePathWatcherInterface() = default;
    ProjectStoragePathWatcherInterface(const ProjectStoragePathWatcherInterface &) = delete;
    ProjectStoragePathWatcherInterface &operator=(const ProjectStoragePathWatcherInterface &) = delete;

    virtual void updateIdPaths(const std::vector<IdPaths> &idPaths) = 0;
    virtual void updateContextIdPaths(const std::vector<IdPaths> &idPaths,
                                      const SourceContextIds &sourceContextIds)
        = 0;
    virtual void removeIds(const ProjectPartIds &ids) = 0;

    virtual void setNotifier(ProjectStoragePathWatcherNotifierInterface *notifier) = 0;

protected:
    ~ProjectStoragePathWatcherInterface() = default;
};

} // namespace QmlDesigner
