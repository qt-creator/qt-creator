// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/environmentfwd.h>
#include <utils/namevaluedictionary.h>

namespace ProjectExplorer { class Kit; }
namespace Utils { class Id; }

namespace McuSupport::Internal {

class McuDependenciesKitAspect final
{
public:
    static Utils::Id id();
    static Utils::EnvironmentItems dependencies(const ProjectExplorer::Kit *kit);
    static void setDependencies(ProjectExplorer::Kit *kit, const Utils::EnvironmentItems &dependencies);
    static Utils::NameValuePairs configuration(const ProjectExplorer::Kit *kit);
};

} // McuSupport::Internal
