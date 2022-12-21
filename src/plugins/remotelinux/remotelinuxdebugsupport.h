// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerruncontrol.h>

namespace RemoteLinux::Internal {

class LinuxDeviceDebugSupport : public Debugger::DebuggerRunTool
{
public:
    LinuxDeviceDebugSupport(ProjectExplorer::RunControl *runControl);
};

} // RemoteLinux::Internal
