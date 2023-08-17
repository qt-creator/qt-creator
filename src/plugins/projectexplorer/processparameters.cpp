// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processparameters.h"

#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/process.h>
#include <utils/theme/theme.h>
#include <utils/utilstr.h>

#include <QDir>

/*!
    \class ProjectExplorer::ProcessParameters

    \brief The ProcessParameters class aggregates all parameters needed to start
    a process.

    It offers a set of functions which expand macros and environment variables
    inside the raw parameters to obtain final values for starting a process
    or for display purposes.

    \sa ProjectExplorer::AbstractProcessStep
*/

using namespace Utils;

namespace ProjectExplorer {

ProcessParameters::ProcessParameters() = default;

/*!
    Sets the command to run.
*/
void ProcessParameters::setCommandLine(const CommandLine &cmdLine)
{
    m_runData.command = cmdLine;
    m_effectiveCommand.clear();
    m_effectiveArguments.clear();

    effectiveCommand();
    effectiveArguments();
}

/*!
    Sets the \a workingDirectory for the process for a build configuration.

    Should be called from init().
*/

void ProcessParameters::setWorkingDirectory(const FilePath &workingDirectory)
{
    m_runData.workingDirectory = workingDirectory;
    m_effectiveWorkingDirectory.clear();

    effectiveWorkingDirectory();
}

/*!
    \fn void ProjectExplorer::ProcessParameters::setEnvironment(const Utils::Environment &env)
    Sets the environment \a env for running the command.

    Should be called from init().
*/

/*!
   \fn  void ProjectExplorer::ProcessParameters::setMacroExpander(Utils::MacroExpander *mx)
   Sets the macro expander \a mx to use on the command, arguments, and working
   dir.

   \note The caller retains ownership of the object.
*/

/*!
    Gets the fully expanded working directory.
*/

FilePath ProcessParameters::effectiveWorkingDirectory() const
{
    if (m_effectiveWorkingDirectory.isEmpty()) {
        m_effectiveWorkingDirectory = m_runData.workingDirectory;
        QString path = m_runData.workingDirectory.path();
        if (m_macroExpander)
            path = m_macroExpander->expand(path);
        m_effectiveWorkingDirectory = m_effectiveWorkingDirectory.withNewPath(
            QDir::cleanPath(m_runData.environment.expandVariables(path)));
    }
    return m_effectiveWorkingDirectory;
}

/*!
    Gets the fully expanded command name to run.
*/

FilePath ProcessParameters::effectiveCommand() const
{
    if (m_effectiveCommand.isEmpty()) {
        FilePath cmd = m_runData.command.executable();
        if (m_macroExpander)
            cmd = m_macroExpander->expand(cmd);
        if (cmd.needsDevice()) {
            // Assume this is already good. FIXME: It is possibly not, so better fix  searchInPath.
            m_effectiveCommand = cmd;
        } else {
            m_effectiveCommand = m_runData.environment.searchInPath(cmd.toString(),
                                                                    {effectiveWorkingDirectory()});
        }
        m_commandMissing = m_effectiveCommand.isEmpty();
        if (m_commandMissing)
            m_effectiveCommand = cmd;
    }
    return m_effectiveCommand;
}

/*!
    Returns \c true if effectiveCommand() would return only a fallback.
*/

bool ProcessParameters::commandMissing() const
{
    effectiveCommand();
    return m_commandMissing;
}

QString ProcessParameters::effectiveArguments() const
{
    if (m_effectiveArguments.isEmpty()) {
        m_effectiveArguments = m_runData.command.arguments();
        if (m_macroExpander)
            m_effectiveArguments = m_macroExpander->expand(m_effectiveArguments);
    }
    return m_effectiveArguments;
}

QString ProcessParameters::prettyCommand() const
{
    QString cmd = m_runData.command.executable().toString();
    if (m_macroExpander)
        cmd = m_macroExpander->expand(cmd);
    return FilePath::fromString(cmd).fileName();
}

QString ProcessParameters::prettyArguments() const
{
    const QString margs = effectiveArguments();
    const FilePath workDir = effectiveWorkingDirectory();
    ProcessArgs::SplitError err;
    const ProcessArgs args = ProcessArgs::prepareArgs(margs, &err, HostOsInfo::hostOs(),
                                                      &m_runData.environment, &workDir);
    if (err != ProcessArgs::SplitOk)
        return margs; // Sorry, too complex - just fall back.
    return args.toString();
}

static QString invalidCommandMessage(const QString &displayName)
{
    return QString("<b>%1:</b> <font color='%3'>%2</font>")
                    .arg(displayName,
                         ::Utils::Tr::tr("Invalid command"),
                         creatorTheme()->color(Theme::TextColorError).name());
}

QString ProcessParameters::summary(const QString &displayName) const
{
    if (m_commandMissing)
        return invalidCommandMessage(displayName);

    return QString::fromLatin1("<b>%1:</b> %2 %3")
            .arg(displayName,
                 ProcessArgs::quoteArg(prettyCommand()),
                 prettyArguments());
}

QString ProcessParameters::summaryInWorkdir(const QString &displayName) const
{
    if (m_commandMissing)
        return invalidCommandMessage(displayName);

    return QString::fromLatin1("<b>%1:</b> %2 %3 in %4")
            .arg(displayName,
                 ProcessArgs::quoteArg(prettyCommand()),
                 prettyArguments(),
                 QDir::toNativeSeparators(effectiveWorkingDirectory().toString()));
}

} // ProcessExplorer
