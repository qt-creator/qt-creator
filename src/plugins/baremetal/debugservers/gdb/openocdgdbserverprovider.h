// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

namespace BareMetal::Internal {

class OpenOcdGdbServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    OpenOcdGdbServerProviderFactory();
};

} // BareMetal::Internal
