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

#include "projectexplorer_export.h"

#include <utils/environment.h>

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProcessParameters
{
public:
    ProcessParameters();

    void setCommand(const QString &cmd);
    QString command() const { return m_command; }

    void setArguments(const QString &arguments);
    QString arguments() const { return m_arguments; }

    void setWorkingDirectory(const QString &workingDirectory);
    QString workingDirectory() const { return m_workingDirectory; }

    void setEnvironment(const Utils::Environment &env) { m_environment = env; }
    Utils::Environment environment() const { return m_environment; }

    void setMacroExpander(Utils::MacroExpander *mx) { m_macroExpander = mx; }
    Utils::MacroExpander *macroExpander() const { return m_macroExpander; }

    /// Get the fully expanded working directory:
    QString effectiveWorkingDirectory() const;
    /// Get the fully expanded command name to run:
    QString effectiveCommand() const;
    /// Get the fully expanded arguments to use:
    QString effectiveArguments() const;

    /// True if effectiveCommand() would return only a fallback
    bool commandMissing() const;

    QString prettyCommand() const;
    QString prettyArguments() const;
    QString summary(const QString &displayName) const;
    QString summaryInWorkdir(const QString &displayName) const;

    void resolveAll();
private:
    QString m_workingDirectory;
    QString m_command;
    QString m_arguments;
    Utils::Environment m_environment;
    Utils::MacroExpander *m_macroExpander;

    mutable QString m_effectiveWorkingDirectory;
    mutable QString m_effectiveCommand;
    mutable QString m_effectiveArguments;
    mutable bool m_commandMissing;
};

} // namespace ProjectExplorer
