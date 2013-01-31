/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "processparameters.h"

#include <utils/stringutils.h>
#include <utils/qtcprocess.h>

#include <QFileInfo>
#include <QDir>

/*!
    \class ProjectExplorer::ProcessParameters

    \brief ProcessParameters aggregates all parameters needed to start a process.

    It offers a set of functions which expand macros and environment variables
    inside the raw parameters to obtain final values for starting a process
    or for display purposes.

    \sa ProjectExplorer::AbstractProcessStep
*/

using namespace ProjectExplorer;

ProcessParameters::ProcessParameters() :
    m_macroExpander(0),
    m_commandMissing(false)
{
}

/*!
    \brief Sets the executable to run.
*/

void ProcessParameters::setCommand(const QString &cmd)
{
    m_command = cmd;
    m_effectiveCommand.clear();
}

/*!
    \brief Sets the command line arguments used by the process
*/

void ProcessParameters::setArguments(const QString &arguments)
{
    m_arguments = arguments;
    m_effectiveArguments.clear();
}

/*!
    \brief Sets the workingDirectory for the process for a buildConfiguration
    should be called from init()
*/

void ProcessParameters::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
    m_effectiveWorkingDirectory.clear();
}

/*!
    \fn void ProjectExplorer::ProcessParameters::setEnvironment(const Utils::Environment &env)
    \brief Set the Environment for running the command

    Should be called from init()
*/

/*!
   \fn  void ProjectExplorer::ProcessParameters::setMacroExpander(Utils::AbstractMacroExpander *mx)
   \brief Set the macro expander to use on the command, arguments and working dir.

   Note that the caller retains ownership of the object.
*/

/*!
    \brief Get the fully expanded working directory.
*/

QString ProcessParameters::effectiveWorkingDirectory() const
{
    if (m_effectiveWorkingDirectory.isEmpty()) {
        QString wds = m_workingDirectory;
        if (m_macroExpander)
            Utils::expandMacros(&wds, m_macroExpander);
        m_effectiveWorkingDirectory = QDir::cleanPath(m_environment.expandVariables(wds));
    }
    return m_effectiveWorkingDirectory;
}

/*!
    \brief Get the fully expanded command name to run
*/

QString ProcessParameters::effectiveCommand() const
{
    if (m_effectiveCommand.isEmpty()) {
        QString cmd = m_command;
        if (m_macroExpander)
            Utils::expandMacros(&cmd, m_macroExpander);
        m_effectiveCommand = QDir::cleanPath(m_environment.searchInPath(
                    cmd, QStringList() << effectiveWorkingDirectory()));
        m_commandMissing = m_effectiveCommand.isEmpty();
        if (m_commandMissing)
            m_effectiveCommand = cmd;
    }
    return m_effectiveCommand;
}

/*!
    \brief True if effectiveCommand() would return only a fallback.
*/

bool ProcessParameters::commandMissing() const
{
    effectiveCommand();
    return m_commandMissing;
}

QString ProcessParameters::effectiveArguments() const
{
    if (m_effectiveArguments.isEmpty()) {
        m_effectiveArguments = m_arguments;
        if (m_macroExpander)
            Utils::expandMacros(&m_effectiveArguments, m_macroExpander);
    }
    return m_effectiveArguments;
}

QString ProcessParameters::prettyCommand() const
{
    QString cmd = m_command;
    if (m_macroExpander)
        Utils::expandMacros(&cmd, m_macroExpander);
    return QFileInfo(cmd).fileName();
}

QString ProcessParameters::prettyArguments() const
{
    QString margs = effectiveArguments();
    QString workDir = effectiveWorkingDirectory();
#ifdef Q_OS_WIN
    QString args;
#else
    QStringList args;
#endif
    Utils::QtcProcess::SplitError err;
    args = Utils::QtcProcess::prepareArgs(margs, &err, &m_environment, &workDir);
    if (err != Utils::QtcProcess::SplitOk)
        return margs; // Sorry, too complex - just fall back.
#ifdef Q_OS_WIN
    return args;
#else
    return Utils::QtcProcess::joinArgs(args);
#endif
}

QString ProcessParameters::summary(const QString &displayName) const
{
    return QString::fromLatin1("<b>%1:</b> %2 %3")
            .arg(displayName,
                 Utils::QtcProcess::quoteArg(prettyCommand()),
                 prettyArguments());
}

QString ProcessParameters::summaryInWorkdir(const QString &displayName) const
{
    return QString::fromLatin1("<b>%1:</b> %2 %3 in %4")
            .arg(displayName,
                 Utils::QtcProcess::quoteArg(prettyCommand()),
                 prettyArguments(),
                 QDir::toNativeSeparators(effectiveWorkingDirectory()));
}

void ProcessParameters::resolveAll()
{
    effectiveCommand();
    effectiveArguments();
    effectiveWorkingDirectory();
}
