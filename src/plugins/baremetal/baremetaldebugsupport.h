// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerruncontrol.h>

namespace BareMetal::Internal {

class BareMetalDebugSupport final : public Debugger::DebuggerRunTool
{
public:
    explicit BareMetalDebugSupport(ProjectExplorer::RunControl *runControl);

private:
    void start() final;
};

} // BareMetal::Internal
