// Copyright (C) 2019 Kovalev Dmitry <kovalevda1991@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

namespace BareMetal::Internal {

class JLinkGdbServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    JLinkGdbServerProviderFactory();
};

} // BareMetal::Internal
