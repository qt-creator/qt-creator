/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "abldparser.h"
#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildparserinterface.h>
#include <projectexplorer/taskwindow.h>

#include <extensionsystem/pluginmanager.h>

using namespace Qt4ProjectManager;
using namespace ProjectExplorer;

AbldParser::AbldParser(const QString &name) :
    m_name(name),
    m_currentLine(-1),
    m_waitingForStdErrContinuation(false),
    m_waitingForStdOutContinuation(false)
{
    m_perlIssue.setPattern("^(WARNING|ERROR):\\s([^\\(\\)]+[^\\d])\\((\\d+)\\) : (.+)$");
    m_perlIssue.setMinimal(true);

    // Now look for new parser
    QList<IBuildParserFactory *> buildParserFactories =
            ExtensionSystem::PluginManager::instance()->getObjects<ProjectExplorer::IBuildParserFactory>();

    QString subparser_name;

    if ((m_name == QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_ABLD_GCCE)))
        subparser_name = QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_GCC);
    else if ((m_name == QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_ABLD_WINSCW)))
        subparser_name = QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_WINSCW);
    else if (m_name == QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_ABLD_RVCT))
        subparser_name = QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_RVCT);

    QTC_ASSERT(!subparser_name.isNull(), return);

    foreach (IBuildParserFactory * factory, buildParserFactories) {
        if (factory->canCreate(subparser_name)) {
            m_subparser = factory->create(subparser_name);
            break;
        }
    }
    QTC_ASSERT(0 != m_subparser, return);

    connect(m_subparser, SIGNAL(enterDirectory(const QString &)),
            this, SIGNAL(enterDirectory(const QString &)));
    connect(m_subparser, SIGNAL(leaveDirectory(const QString &)),
            this, SIGNAL(leaveDirectory(const QString &)));
    connect(m_subparser, SIGNAL(addToOutputWindow(const QString &)),
            this, SIGNAL(addToOutputWindow(const QString &)));
    connect(m_subparser, SIGNAL(addToTaskWindow(const QString &, int, int, const QString &)),
            this, SIGNAL(addToTaskWindow(const QString &, int, int, const QString &)));
}

AbldParser::~AbldParser()
{
    delete m_subparser;
}

QString AbldParser::name() const
{
    return m_name;
}

void AbldParser::stdOutput(const QString &line)
{
    m_waitingForStdErrContinuation = false;

    QString lne = line.trimmed();
    // possible ABLD.bat errors:
    if (lne.startsWith("Is Perl, version ")) {
        emit addToTaskWindow(
                QString(), //filename
                TaskWindow::Error,
                -1, //linenumber
                lne);
        return;
    }
    if (lne.startsWith("FATAL ERROR:")) {
        emit addToTaskWindow(QString(), TaskWindow::Error,
                             -1, lne.mid(12));
        m_waitingForStdOutContinuation = false;
        return;
    }

    if (m_perlIssue.indexIn(lne) > -1) {
        m_waitingForStdOutContinuation = true;
        TaskWindow::TaskType type;
        if (m_perlIssue.cap(1) == QLatin1String("WARNING"))
            type = TaskWindow::Warning;
        else if (m_perlIssue.cap(1) == QLatin1String("ERROR"))
            type = TaskWindow::Error;
        else
            type = TaskWindow::Unknown;

        m_currentFile = m_perlIssue.cap(2);
        m_currentLine = m_perlIssue.cap(3).toInt();

        emit addToTaskWindow(m_currentFile, type,
                             m_currentLine, m_perlIssue.cap(4));
        return;
    }

    if (lne.isEmpty()) {
        m_waitingForStdOutContinuation = false;
        return;
    }

    if (m_waitingForStdOutContinuation) {
        emit addToTaskWindow(m_currentFile,
                             TaskWindow::Unknown,
                             m_currentLine, lne);
        m_waitingForStdOutContinuation = true;
        return;
    }

    QTC_ASSERT(0 != m_subparser, return);
    // pass on to compiler output parser:
    m_subparser->stdOutput(lne);
}

void AbldParser::stdError(const QString &line)
{
    m_waitingForStdOutContinuation = false;

    QString lne = line.trimmed();

    // possible abld.pl errors:
    if (lne.startsWith("ABLD ERROR:") ||
        lne.startsWith("This project does not support ") ||
        lne.startsWith("Platform ")) {
        emit addToTaskWindow(
                QString(), // filename,
                TaskWindow::Error,
                -1, // linenumber
                lne);
        return;
    }

    if (lne.startsWith("Died at ")) {
        emit addToTaskWindow(QString(),
                             TaskWindow::Error,
                             -1, lne);
        m_waitingForStdErrContinuation = false;
        return;
    }

    if (lne.startsWith("MMPFILE \"")) {
        m_currentFile = lne.mid(9, lne.size() - 10);
        m_waitingForStdErrContinuation = false;
        return;
    }
    if (lne.isEmpty()) {
        m_waitingForStdErrContinuation = false;
        return;
    }
    if (lne.startsWith("WARNING: ")) {
        QString description = lne.mid(9);
        emit addToTaskWindow(m_currentFile,
                             TaskWindow::Warning,
                             -1, description);
        m_waitingForStdErrContinuation = true;
        return;
    }
    if (lne.startsWith("ERROR: ")) {
        QString description = lne.mid(7);
        emit addToTaskWindow(m_currentFile,
                             TaskWindow::Error,
                             -1, description);
        m_waitingForStdErrContinuation = true;
        return;
    }
    if (m_waitingForStdErrContinuation)
    {
        emit addToTaskWindow(m_currentFile,
                             TaskWindow::Unknown,
                             -1, lne);
        m_waitingForStdErrContinuation = true;
        return;
    }

    QTC_ASSERT(0 != m_subparser, return);
    // pass on to compiler output parser:
    m_subparser->stdError(lne);
}
