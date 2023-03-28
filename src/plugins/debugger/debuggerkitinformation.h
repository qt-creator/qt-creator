// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfiguration.h>

namespace Debugger {
class DebuggerItem;

class DEBUGGER_EXPORT DebuggerKitAspect : public ProjectExplorer::KitAspect
{
public:
    DebuggerKitAspect();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const override
        { return DebuggerKitAspect::validateDebugger(k); }

    void setup(ProjectExplorer::Kit *k) override;

    static const DebuggerItem *debugger(const ProjectExplorer::Kit *kit);
    static ProjectExplorer::Runnable runnable(const ProjectExplorer::Kit *kit);

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

    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *k) const override;
    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;

    ItemList toUserOutput(const ProjectExplorer::Kit *k) const override;

    static void setDebugger(ProjectExplorer::Kit *k, const QVariant &id);

    static Utils::Id id();
    static DebuggerEngineType engineType(const ProjectExplorer::Kit *k);
    static QString displayString(const ProjectExplorer::Kit *k);
};

} // Debugger
