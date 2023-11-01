// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace Haskell::Internal {

class HaskellRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    HaskellRunConfigurationFactory();
};

} // Haskell::Internal
