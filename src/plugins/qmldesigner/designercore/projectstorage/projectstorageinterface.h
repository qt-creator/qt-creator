// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "projectstoragetypes.h"

namespace QmlDesigner {

class ProjectStorageInterface
{
public:
    virtual void synchronize(Storage::Synchronization::SynchronizationPackage package) = 0;

    virtual ModuleId moduleId(Utils::SmallStringView name) const = 0;

    virtual FileStatus fetchFileStatus(SourceId sourceId) const = 0;
    virtual Storage::Synchronization::ProjectDatas fetchProjectDatas(SourceId sourceId) const = 0;

protected:
    ~ProjectStorageInterface() = default;
};

} // namespace QmlDesigner
