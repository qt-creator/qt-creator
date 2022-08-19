// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    AutotoolsBuildConfigurationFactory();
};

} // namespace Internal
} // namespace AutotoolsProjectManager
