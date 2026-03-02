// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/environmentfwd.h>

namespace Utils { class Id; }

namespace ProjectExplorer {
class Kit;

class PROJECTEXPLORER_EXPORT EnvironmentKitAspect
{
public:
    static Utils::Id id();

    static Utils::EnvironmentChanges buildEnvChanges(const Kit *k);
    static void setBuildEnvChanges(Kit *k, const Utils::EnvironmentChanges &changes);

    static Utils::EnvironmentChanges runEnvChanges(const Kit *k);
    static void setRunEnvChanges(Kit *k, const Utils::EnvironmentChanges &changes);
};

} // namespace ProjectExplorer
