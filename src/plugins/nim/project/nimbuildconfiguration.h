// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>

namespace Nim {

class NimBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    NimBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

public:
    Utils::FilePath cacheDirectory() const;
    Utils::FilePath outFilePath() const;
};


class NimBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    NimBuildConfigurationFactory();
};

} // Nim
