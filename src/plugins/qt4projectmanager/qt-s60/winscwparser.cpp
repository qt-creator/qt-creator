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

using namespace Qt4ProjectManager;

WinscwParser::WinscwParser()
{
    // linker problems:
    m_linkerProblem.setPattern("^(\\S*)\\(\\S+\\):\\s(.+)$");
    m_linkerProblem.setMinimal(true);

    //make[4]: Entering directory `/home/kkoehne/dev/ide-explorer/src/plugins/qtscripteditor'
    m_makeDir.setPattern("^make.*: (\\w+) directory .(.+).$");
    m_makeDir.setMinimal(true);
}

QString WinscwParser::name() const
{
    return QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_RVCT);
}

void WinscwParser::stdOutput(const QString &line)
{
    QString lne = line.trimmed();

    if (m_makeDir.indexIn(lne) > -1) {
        if (m_makeDir.cap(1) == "Leaving")
            emit leaveDirectory(m_makeDir.cap(2));
        else
            emit enterDirectory(m_makeDir.cap(2));
    }

    if (!m_fileName.isNull() &&
        lne.startsWith(m_fileName))
    {
        int file_name_size(m_fileName.size());
        int second_colon = lne.indexOf(':', file_name_size + 1);
        if (second_colon < 0)
        {
            m_fileName = lne + ':';
            return;
        }

        QString file_name(lne.left(file_name_size - 1));
        QString line_info(lne.mid(file_name_size, second_colon - file_name_size));
        QString description(lne.mid(second_colon + 1));

        emit addToTaskWindow(
                file_name,
                ProjectExplorer::BuildParserInterface::Error,
                line_info.toInt(),
                description);
        return;
    }

    m_fileName = lne + ':';
}

void WinscwParser::stdError(const QString &line)
{
    QString lne = line.trimmed();
    if (m_linkerProblem.indexIn(lne) > -1) {
       QString description = m_linkerProblem.cap(2);
       emit addToTaskWindow(
           m_linkerProblem.cap(1), //filename
           ProjectExplorer::BuildParserInterface::Error,
           -1, //linenumber
           description);
   }
}
