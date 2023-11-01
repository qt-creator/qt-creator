// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesontools.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

namespace MesonProjectManager::Internal {

class MesonToolKitAspect final
{
public:
    static void setMesonTool(ProjectExplorer::Kit *kit, Utils::Id id);
    static Utils::Id mesonToolId(const ProjectExplorer::Kit *kit);

    static inline decltype(auto) mesonTool(const ProjectExplorer::Kit *kit)
    {
        return MesonTools::mesonWrapper(MesonToolKitAspect::mesonToolId(kit));
    }

    static inline bool isValid(const ProjectExplorer::Kit *kit)
    {
        auto tool = mesonTool(kit);
        return (tool && tool->isValid());
    }
};

} // MesonProjectManager::Internal
