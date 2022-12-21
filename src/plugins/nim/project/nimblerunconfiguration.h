// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace Nim {

class NimbleRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    NimbleRunConfigurationFactory();
};

class NimbleTestConfigurationFactory final : public ProjectExplorer::FixedRunConfigurationFactory
{
public:
    NimbleTestConfigurationFactory();
};

} // Nim
