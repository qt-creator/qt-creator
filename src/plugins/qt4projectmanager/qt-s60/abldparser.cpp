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

#include "abldparser.h"

#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskwindow.h>

using namespace Qt4ProjectManager;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

AbldParser::AbldParser() :
    m_currentLine(-1),
    m_waitingForStdErrContinuation(false),
    m_waitingForStdOutContinuation(false)
{
    m_perlIssue.setPattern("^(WARNING|ERROR):\\s([^\\(\\)]+[^\\d])\\((\\d+)\\) : (.+)$");
    m_perlIssue.setMinimal(true);

    appendOutputParser(new GnuMakeParser);
}

void AbldParser::stdOutput(const QString &line)
{
    m_waitingForStdErrContinuation = false;

    QString lne = line.trimmed();
    // possible ABLD.bat errors:
    if (lne.startsWith(QLatin1String("Is Perl, version "))) {
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      lne /* description */,
                                      QString() /* filename */,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_BUILDSYSTEM));
        return;
    }
    if (lne.startsWith(QLatin1String("FATAL ERROR:"))) {
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      lne /* description */,
                                      QString() /* filename */,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_BUILDSYSTEM));
        m_waitingForStdOutContinuation = false;
        return;
    }

    if (m_perlIssue.indexIn(lne) > -1) {
        m_waitingForStdOutContinuation = true;
        m_currentFile = m_perlIssue.cap(2);
        m_currentLine = m_perlIssue.cap(3).toInt();

        TaskWindow::Task task(TaskWindow::Unknown,
                              m_perlIssue.cap(4) /* description */,
                              m_currentFile, m_currentLine,
                              TASK_CATEGORY_BUILDSYSTEM);

        if (m_perlIssue.cap(1) == QLatin1String("WARNING"))
            task.type = TaskWindow::Warning;
        else if (m_perlIssue.cap(1) == QLatin1String("ERROR"))
            task.type = TaskWindow::Error;

        emit addTask(task);
        return;
    }

    if (lne.isEmpty()) {
        m_waitingForStdOutContinuation = false;
        return;
    }

    if (m_waitingForStdOutContinuation) {
        emit addTask(TaskWindow::Task(TaskWindow::Unknown,
                                      lne /* description */,
                                      m_currentFile, m_currentLine,
                                      TASK_CATEGORY_BUILDSYSTEM));
        m_waitingForStdOutContinuation = true;
        return;
    }
    IOutputParser::stdOutput(line);
}

void AbldParser::stdError(const QString &line)
{
    m_waitingForStdOutContinuation = false;

    QString lne = line.trimmed();

    // possible abld.pl errors:
    if (lne.startsWith(QLatin1String("ABLD ERROR:")) ||
        lne.startsWith(QLatin1String("This project does not support ")) ||
        lne.startsWith(QLatin1String("Platform "))) {
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      lne /* description */,
                                      QString() /* filename */,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_BUILDSYSTEM));
        return;
    }

    if (lne.startsWith(QLatin1String("Died at "))) {
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      lne /* description */,
                                      QString() /* filename */,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_BUILDSYSTEM));
        m_waitingForStdErrContinuation = false;
        return;
    }

    if (lne.startsWith(QLatin1String("MMPFILE \""))) {
        m_currentFile = lne.mid(9, lne.size() - 10);
        m_waitingForStdErrContinuation = false;
        return;
    }
    if (lne.isEmpty()) {
        m_waitingForStdErrContinuation = false;
        return;
    }
    if (lne.startsWith(QLatin1String("WARNING: "))) {
        QString description = lne.mid(9);
        emit addTask(TaskWindow::Task(TaskWindow::Warning, description,
                                      m_currentFile,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_BUILDSYSTEM));
        m_waitingForStdErrContinuation = true;
        return;
    }
    if (lne.startsWith(QLatin1String("ERROR: "))) {
        QString description = lne.mid(7);
        emit addTask(TaskWindow::Task(TaskWindow::Error, description,
                                      m_currentFile,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_BUILDSYSTEM));
        m_waitingForStdErrContinuation = true;
        return;
    }
    if (m_waitingForStdErrContinuation)
    {
        emit addTask(TaskWindow::Task(TaskWindow::Unknown,
                                      lne /* description */,
                                      m_currentFile,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_BUILDSYSTEM));
        m_waitingForStdErrContinuation = true;
        return;
    }
    IOutputParser::stdError(line);
}
