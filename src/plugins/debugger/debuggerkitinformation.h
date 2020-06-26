/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfiguration.h>

namespace Debugger {
class DebuggerItem;

class DEBUGGER_EXPORT DebuggerKitAspect : public ProjectExplorer::KitAspect
{
    Q_OBJECT

public:
    DebuggerKitAspect();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const override
        { return DebuggerKitAspect::validateDebugger(k); }

    void setup(ProjectExplorer::Kit *k) override;
    void fix(ProjectExplorer::Kit *k) override;

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

} // namespace Debugger
