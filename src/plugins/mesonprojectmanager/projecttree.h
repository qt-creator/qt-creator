// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesoninfoparser.h"
#include "mesonprojectnodes.h"

#include <utils/fileutils.h>

namespace MesonProjectManager {
namespace Internal {

class ProjectTree
{
public:
    ProjectTree();
    static std::unique_ptr<MesonProjectNode> buildTree(const Utils::FilePath &srcDir,
                                                       const TargetsList &targets,
                                                       const std::vector<Utils::FilePath> &bsFiles);
};

} // namespace Internal
} // namespace MesonProjectManager
