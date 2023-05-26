// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace MesonProjectManager::Internal {

class MesonRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    MesonRunConfigurationFactory();
};

} // MesonProjectManager::Internal
