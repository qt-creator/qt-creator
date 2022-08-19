// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerruncontrol.h>

namespace ProjectExplorer {
class RunControl;
}

namespace BareMetal {
namespace Internal {

class IDebugServerProvider;

// BareMetalDebugSupport

class BareMetalDebugSupport final : public Debugger::DebuggerRunTool
{
    Q_OBJECT

public:
    explicit BareMetalDebugSupport(ProjectExplorer::RunControl *runControl);

private:
    void start() final;
};

} // namespace Internal
} // namespace BareMetal
