// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerruncontrol.h>

namespace Qnx::Internal {

class QnxDebugSupport : public Debugger::DebuggerRunTool
{
public:
    explicit QnxDebugSupport(ProjectExplorer::RunControl *runControl);
};

class QnxAttachDebugSupport : public Debugger::DebuggerRunTool
{
public:
    explicit QnxAttachDebugSupport(ProjectExplorer::RunControl *runControl);

    static void showProcessesDialog();
};

} // Qnx::Internal
