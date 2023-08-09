// Copyright (C) 2019 Andrey Sobol <andrey.sobol.nn@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

namespace BareMetal::Internal {

class EBlinkGdbServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    EBlinkGdbServerProviderFactory();
};

} // BareMetal::Internal
