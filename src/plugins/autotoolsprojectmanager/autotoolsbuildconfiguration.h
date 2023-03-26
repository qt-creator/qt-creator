// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>

namespace AutotoolsProjectManager::Internal {

class AutotoolsBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    AutotoolsBuildConfigurationFactory();
};

} // AutotoolsProjectManager::Internal
