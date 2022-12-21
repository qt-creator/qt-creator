// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace MesonProjectManager {
namespace Internal {

class MesonRunConfiguration final : public ProjectExplorer::RunConfiguration
{
public:
    MesonRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);

private:
    void updateTargetInformation();
};

class MesonRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    MesonRunConfigurationFactory();
};

} // namespace Internal
} // namespace MesonProjectManager
