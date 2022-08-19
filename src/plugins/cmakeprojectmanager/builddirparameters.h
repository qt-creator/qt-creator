// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cmaketool.h"

#include <utils/environment.h>
#include <utils/filepath.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildSystem;

class BuildDirParameters
{
public:
    BuildDirParameters();
    explicit BuildDirParameters(CMakeBuildSystem *buildSystem);

    bool isValid() const;
    CMakeTool *cmakeTool() const;

    QString projectName;

    Utils::FilePath sourceDirectory;
    Utils::FilePath buildDirectory;
    QString cmakeBuildType;

    Utils::Environment environment;

    Utils::Id cmakeToolId;

    QStringList initialCMakeArguments;
    QStringList configurationChangesArguments;
    QStringList additionalCMakeArguments;
};

} // namespace Internal
} // namespace CMakeProjectManager
