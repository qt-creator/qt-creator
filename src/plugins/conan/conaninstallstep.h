// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace Conan::Internal {

class ConanInstallStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    ConanInstallStepFactory();
};

} // Conan::Internal
