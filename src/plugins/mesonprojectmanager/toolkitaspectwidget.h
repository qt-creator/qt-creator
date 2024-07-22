// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "toolwrapper.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

namespace MesonProjectManager::Internal {

class MesonToolKitAspect final
{
public:
    static void setMesonTool(ProjectExplorer::Kit *kit, Utils::Id id);
    static Utils::Id mesonToolId(const ProjectExplorer::Kit *kit);

    static bool isValid(const ProjectExplorer::Kit *kit);
    static std::shared_ptr<ToolWrapper> mesonTool(const ProjectExplorer::Kit *kit);
};

class NinjaToolKitAspect final
{
public:
    static void setNinjaTool(ProjectExplorer::Kit *kit, Utils::Id id);
    static Utils::Id ninjaToolId(const ProjectExplorer::Kit *kit);

    static bool isValid(const ProjectExplorer::Kit *kit);
    static std::shared_ptr<ToolWrapper> ninjaTool(const ProjectExplorer::Kit *kit);
};

} // MesonProjectManager::Internal
