// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesontools.h"

namespace ProjectExplorer { class Kit; }

namespace MesonProjectManager::Internal {

class MesonToolKitAspect final
{
public:
    static void setMesonTool(ProjectExplorer::Kit *kit, Utils::Id id);
    static Utils::Id mesonToolId(const ProjectExplorer::Kit *kit);

    static bool isValid(const ProjectExplorer::Kit *kit);
    static std::shared_ptr<MesonToolWrapper> mesonTool(const ProjectExplorer::Kit *kit);
};


} // MesonProjectManager::Internal
