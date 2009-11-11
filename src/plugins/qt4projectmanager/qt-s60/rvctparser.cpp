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

#include "rvctparser.h"
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskwindow.h>

using namespace ProjectExplorer;
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

    //make[4]: Entering directory `/home/kkoehne/dev/ide-explorer/src/plugins/qtscripteditor'
    m_makeDir.setPattern("^make.*: (\\w+) directory .(.+).$");
    m_makeDir.setMinimal(true);
}

QString RvctParser::name() const
{
    return QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_RVCT);
}

void RvctParser::stdOutput(const QString &line)
{
    QString lne = line.trimmed();

    if (m_makeDir.indexIn(lne) > -1) {
        if (m_makeDir.cap(1) == "Leaving")
            emit leaveDirectory(m_makeDir.cap(2));
        else
            emit enterDirectory(m_makeDir.cap(2));
    }
}

void RvctParser::stdError(const QString &line)
{
    QString lne = line.trimmed();

    if (m_linkerProblem.indexIn(lne) > -1) {
       QString description = m_linkerProblem.cap(2);
       emit addToTaskWindow(
           m_linkerProblem.cap(1), //filename
           TaskWindow::Error,
           -1, //linenumber
           description);
   } else if (m_warningOrError.indexIn(lne) > -1) {
       TaskWindow::TaskType type;
       if (m_warningOrError.cap(4) == "Warning")
           type = TaskWindow::Warning;
       else if (m_warningOrError.cap(4) == "Error")
           type = TaskWindow::Error;
       else
           type = TaskWindow::Unknown;

       QString description =  m_warningOrError.cap(5);

       m_additionalInfo = true;
       m_lastFile = m_warningOrError.cap(1);
       m_lastLine = m_warningOrError.cap(2).toInt();

       emit addToTaskWindow(
           m_lastFile, //filename
           type,
           m_lastLine, //line number
           description);
   } else if (m_doneWithFile.indexIn(lne) > -1) {
       m_additionalInfo = false;
   } else if (m_additionalInfo) {
       // Report any lines after a error/warning message as these contain
       // additional information on the problem.
       emit addToTaskWindow(
           m_lastFile, //filesname
           TaskWindow::Unknown,
           m_lastLine, //linenumber
           lne //description
       );
   }
}
