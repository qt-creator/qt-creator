// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>

namespace Nim {

class NimbleBuildStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    NimbleBuildStepFactory();
};

} // Nim
