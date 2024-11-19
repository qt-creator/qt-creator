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
    static Utils::EnvironmentItems environmentChanges(const Kit *k);
    static void setEnvironmentChanges(Kit *k, const Utils::EnvironmentItems &changes);
};

} // namespace ProjectExplorer
