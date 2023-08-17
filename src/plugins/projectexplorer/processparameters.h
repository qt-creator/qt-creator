// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/processinterface.h>

namespace Utils {
class MacroExpander;
} // Utils

namespace ProjectExplorer {

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProcessParameters
{
public:
    ProcessParameters();

    void setCommandLine(const Utils::CommandLine &cmdLine);
    Utils::CommandLine command() const { return m_runData.command; }

    void setWorkingDirectory(const Utils::FilePath &workingDirectory);
    Utils::FilePath workingDirectory() const { return m_runData.workingDirectory; }

    void setEnvironment(const Utils::Environment &env) { m_runData.environment = env; }
    Utils::Environment environment() const { return m_runData.environment; }

    void setMacroExpander(Utils::MacroExpander *mx) { m_macroExpander = mx; }
    Utils::MacroExpander *macroExpander() const { return m_macroExpander; }

    /// Get the fully expanded working directory:
    Utils::FilePath effectiveWorkingDirectory() const;
    /// Get the fully expanded command name to run:
    Utils::FilePath effectiveCommand() const;
    /// Get the fully expanded arguments to use:
    QString effectiveArguments() const;

    /// True if effectiveCommand() would return only a fallback
    bool commandMissing() const;

    QString prettyCommand() const;
    QString prettyArguments() const;
    QString summary(const QString &displayName) const;
    QString summaryInWorkdir(const QString &displayName) const;

private:
    Utils::ProcessRunData m_runData;
    Utils::MacroExpander *m_macroExpander = nullptr;

    mutable Utils::FilePath m_effectiveWorkingDirectory;
    mutable Utils::FilePath m_effectiveCommand;
    mutable QString m_effectiveArguments;
    mutable bool m_commandMissing = false;
};

} // namespace ProjectExplorer
