// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "runconfiguration.h"

namespace ProjectExplorer::Internal {

class DesktopQmakeRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    DesktopQmakeRunConfigurationFactory();
};

class QbsRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    QbsRunConfigurationFactory();
};

class CMakeRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    CMakeRunConfigurationFactory();
};

} // ProjectExplorer::Internal
