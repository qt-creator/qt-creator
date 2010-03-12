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

#include "rvctparser.h"
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskwindow.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;
using namespace Qt4ProjectManager;

RvctParser::RvctParser() :
    m_additionalInfo(false)
{
    // Start of a error or warning:
    m_warningOrError.setPattern("^\"([^\\(\\)]+[^\\d])\", line (\\d+):(\\s(Warning|Error):)\\s(.+)$");
    m_warningOrError.setMinimal(true);

    // Last message for any file with warnings/errors.
    m_doneWithFile.setPattern("^([^\\(\\)]+[^\\d]):\\s(\\d+) warnings?,\\s(\\d+) errors?$");
    m_doneWithFile.setMinimal(true);

    // linker problems:
    m_linkerProblem.setPattern("^(\\S*)\\(\\S+\\):\\s(.+)$");
    m_linkerProblem.setMinimal(true);
}

void RvctParser::stdError(const QString &line)
{
    QString lne = line.trimmed();
    if (m_linkerProblem.indexIn(lne) > -1) {
       emit addTask(Task(Task::Error,
                         m_linkerProblem.cap(2) /* description */,
                         m_linkerProblem.cap(1) /* filename */,
                         -1 /* linenumber */,
                         TASK_CATEGORY_COMPILE));
       return;
   }

   if (m_warningOrError.indexIn(lne) > -1) {
       m_lastFile = m_warningOrError.cap(1);
       m_lastLine = m_warningOrError.cap(2).toInt();

       Task task(Task::Unknown,
                 m_warningOrError.cap(5) /* description */,
                 m_lastFile, m_lastLine,
                 TASK_CATEGORY_COMPILE);
       if (m_warningOrError.cap(4) == "Warning")
           task.type = Task::Warning;
       else if (m_warningOrError.cap(4) == "Error")
           task.type = Task::Error;

       m_additionalInfo = true;

       emit addTask(task);
       return;
   }

   if (m_doneWithFile.indexIn(lne) > -1) {
       m_additionalInfo = false;
       return;
   }
   if (m_additionalInfo) {
       // Report any lines after a error/warning message as these contain
       // additional information on the problem.
       emit addTask(Task(Task::Unknown, lne,
                         m_lastFile,  m_lastLine,
                         TASK_CATEGORY_COMPILE));
       return;
   }
   IOutputParser::stdError(line);
}
