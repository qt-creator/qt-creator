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

#include "winscwparser.h"
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskwindow.h>

using namespace Qt4ProjectManager;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

WinscwParser::WinscwParser()
{
    // linker problems:
    m_linkerProblem.setPattern("^(\\S*)\\(\\S+\\):\\s(.+)$");
    m_linkerProblem.setMinimal(true);

    // WINSCW issue:
    m_compilerProblem.setPattern("^([^\\(\\)]+[^\\d]):(\\d+):\\s(.+)$");
    m_compilerProblem.setMinimal(true);
}

void WinscwParser::stdOutput(const QString &line)
{
    QString lne = line.trimmed();

    if (m_compilerProblem.indexIn(lne) > -1) {
        TaskWindow::Task task(TaskWindow::Error,
                              m_compilerProblem.cap(3) /* description */,
                              m_compilerProblem.cap(1) /* filename */,
                              m_compilerProblem.cap(2).toInt() /* linenumber */,
                              TASK_CATEGORY_COMPILE);
        if (task.description.startsWith("warning: ")) {
            task.type = TaskWindow::Warning;
            task.description = task.description.mid(9);
        }
        emit addTask(task);
        return;
    }
    IOutputParser::stdOutput(line);
}

void WinscwParser::stdError(const QString &line)
{
    QString lne = line.trimmed();

    if (m_linkerProblem.indexIn(lne) > -1) {
        emit addTask(TaskWindow::Task(TaskWindow::Error,
                                      m_linkerProblem.cap(2) /* description */,
                                      m_linkerProblem.cap(1) /* filename */,
                                      -1 /* linenumber */,
                                      TASK_CATEGORY_COMPILE));
        return;
    }
    IOutputParser::stdError(line);
}
