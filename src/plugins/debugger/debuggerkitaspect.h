// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/kitaspects.h>

namespace Utils { class ProcessRunData; }

namespace Debugger {

class DEBUGGER_EXPORT DebuggerKitAspect
{
public:
    enum ConfigurationError
    {
        NoConfigurationError      = 0x0,
        NoDebugger                = 0x1,
        DebuggerNotFound          = 0x2,
        DebuggerNotExecutable     = 0x4,
        DebuggerNeedsAbsolutePath = 0x8,
        DebuggerDoesNotMatch      = 0x10
    };
    Q_DECLARE_FLAGS(ConfigurationErrors, ConfigurationError)

    static ProjectExplorer::Tasks validateDebugger(const ProjectExplorer::Kit *k);
    static ConfigurationErrors configurationErrors(const ProjectExplorer::Kit *k);
    static const class DebuggerItem *debugger(const ProjectExplorer::Kit *kit);
    static Utils::ProcessRunData runnable(const ProjectExplorer::Kit *kit);
    static void setDebugger(ProjectExplorer::Kit *k, const QVariant &id);
    static DebuggerEngineType engineType(const ProjectExplorer::Kit *k);
    static QString displayString(const ProjectExplorer::Kit *k);
    static QString version(const ProjectExplorer::Kit *k);
    static Utils::Id id();
};

} // Debugger
