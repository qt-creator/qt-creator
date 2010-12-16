/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PROCESSPARAMETERS_H
#define PROCESSPARAMETERS_H

#include "projectexplorer_export.h"

#include <utils/environment.h>

namespace Utils {
class AbstractMacroExpander;
}

namespace ProjectExplorer {

/*!
  ProcessParameters aggregates all parameters needed to start a process.

  It offers a set of functions which expand macros and environment variables
  inside the raw parameters to obtain final values for starting a process
  or for display purposes.
*/

class PROJECTEXPLORER_EXPORT ProcessParameters
{
public:
    ProcessParameters();

    /// setCommand() sets the executable to run
    void setCommand(const QString &cmd);
    QString command() const { return m_command; }

    /// sets the command line arguments used by the process
    void setArguments(const QString &arguments);
    QString arguments() const { return m_arguments; }

    /// sets the workingDirectory for the process for a buildConfiguration
    /// should be called from init()
    void setWorkingDirectory(const QString &workingDirectory);
    QString workingDirectory() const { return m_workingDirectory; }

    /// Set the Environment for running the command
    /// should be called from init()
    void setEnvironment(const Utils::Environment &env) { m_environment = env; }
    Utils::Environment environment() const { return m_environment; }

    /// Set the macro expander to use on the command, arguments and working dir.
    /// Note that the caller retains ownership of the object.
    void setMacroExpander(Utils::AbstractMacroExpander *mx) { m_macroExpander = mx; }
    Utils::AbstractMacroExpander *macroExpander() const { return m_macroExpander; }

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

private:
    QString m_workingDirectory;
    QString m_command;
    QString m_arguments;
    Utils::Environment m_environment;
    Utils::AbstractMacroExpander *m_macroExpander;

    mutable QString m_effectiveWorkingDirectory;
    mutable QString m_effectiveCommand;
    mutable QString m_effectiveArguments;
    mutable bool m_commandMissing;
};

} // namespace ProjectExplorer

#endif // PROCESSPARAMETERS_H
