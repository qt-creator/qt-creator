/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
