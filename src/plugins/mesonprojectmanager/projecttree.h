// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesoninfoparser.h"
#include "mesonprojectnodes.h"

#include <utils/filepath.h>

namespace MesonProjectManager::Internal {

class ProjectTree
{
public:
    ProjectTree();
    static std::unique_ptr<MesonProjectNode> buildTree(const Utils::FilePath &srcDir,
                                                       const TargetsList &targets,
                                                       const Utils::FilePaths &bsFiles);
};

} // MesonProjectManager::Internal
