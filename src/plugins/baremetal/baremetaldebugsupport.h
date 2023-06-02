// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace BareMetal::Internal {

class BareMetalDebugSupportFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    BareMetalDebugSupportFactory();
};

} // BareMetal::Internal
