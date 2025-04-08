// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

namespace QmlDesigner {

class ProjectStorageTriggerUpdateInterface
{
public:
    ProjectStorageTriggerUpdateInterface() = default;
    ProjectStorageTriggerUpdateInterface(const ProjectStorageTriggerUpdateInterface &) = delete;
    ProjectStorageTriggerUpdateInterface &operator=(const ProjectStorageTriggerUpdateInterface &) = delete;

    virtual void checkForChangeInDirectory(DirectoryPathIds directoryPathIds) = 0;

protected:
    ~ProjectStorageTriggerUpdateInterface() = default;
};

} // namespace QmlDesigner
