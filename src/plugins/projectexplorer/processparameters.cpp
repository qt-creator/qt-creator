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

#include "processparameters.h"

#include <utils/stringutils.h>
#include <utils/qtcprocess.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>

using namespace ProjectExplorer;

ProcessParameters::ProcessParameters() :
    m_macroExpander(0),
    m_commandMissing(false)
{
}

void ProcessParameters::setCommand(const QString &cmd)
{
    m_command = cmd;
    m_effectiveCommand.clear();
}

void ProcessParameters::setArguments(const QString &arguments)
{
    m_arguments = arguments;
    m_effectiveArguments.clear();
}

void ProcessParameters::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
    m_effectiveWorkingDirectory.clear();
}

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
