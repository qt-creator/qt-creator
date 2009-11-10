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

#include "gccparser.h"
#include "projectexplorerconstants.h"

using namespace ProjectExplorer;

GccParser::GccParser()
{
    m_regExp.setPattern("^([^\\(\\)]+[^\\d]):(\\d+):(\\d+:)*(\\s(warning|error):)?\\s(.+)$");
    m_regExp.setMinimal(true);

    m_regExpIncluded.setPattern("^.*from\\s([^:]+):(\\d+)(,|:)$");
    m_regExpIncluded.setMinimal(true);

    m_regExpLinker.setPattern("^(\\S*)\\(\\S+\\):\\s(.+)$");
    m_regExpLinker.setMinimal(true);

    //make[4]: Entering directory `/home/kkoehne/dev/ide-explorer/src/plugins/qtscripteditor'
    m_makeDir.setPattern("^make.*: (\\w+) directory .(.+).$");
    m_makeDir.setMinimal(true);
}

QString GccParser::name() const
{
    return QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_GCC);
}

void GccParser::stdOutput(const QString & line)
{
    QString lne = line.trimmed();

    if (m_makeDir.indexIn(lne) > -1) {
        if (m_makeDir.cap(1) == "Leaving")
                emit leaveDirectory(m_makeDir.cap(2));
            else
                emit enterDirectory(m_makeDir.cap(2));
    }
}

void GccParser::stdError(const QString & line)
{
    QString lne = line.trimmed();
    if (m_regExpLinker.indexIn(lne) > -1) {
        QString description = m_regExpLinker.cap(2);
        emit addToTaskWindow(
                m_regExpLinker.cap(1), //filename
                ProjectExplorer::BuildParserInterface::Error,
                -1, //linenumber
                description);
    } else if (m_regExp.indexIn(lne) > -1) {
        ProjectExplorer::BuildParserInterface::PatternType type;
        if (m_regExp.cap(5) == "warning")
            type = ProjectExplorer::BuildParserInterface::Warning;
        else if (m_regExp.cap(5) == "error")
            type = ProjectExplorer::BuildParserInterface::Error;
        else
            type = ProjectExplorer::BuildParserInterface::Unknown;

        QString description =  m_regExp.cap(6);

        emit addToTaskWindow(
                m_regExp.cap(1), //filename
                type,
                m_regExp.cap(2).toInt(), //line number
                description);
    } else if (m_regExpIncluded.indexIn(lne) > -1) {
        emit addToTaskWindow(
                m_regExpIncluded.cap(1), //filename
                ProjectExplorer::BuildParserInterface::Unknown,
                m_regExpIncluded.cap(2).toInt(), //linenumber
                lne //description
                );
    } else if (lne.startsWith(QLatin1String("collect2:"))) {
        emit addToTaskWindow("", ProjectExplorer::BuildParserInterface::Error, -1, lne);
    } else if (lne.startsWith(QLatin1String("ERROR:"))) {
        // Triggered by cpp on windows.
        emit addToTaskWindow(QString(), ProjectExplorer::BuildParserInterface::Error, -1, lne);
    } else if (lne == QLatin1String("* cpp failed")) {
        // Triggered by cpp/make on windows.
        emit addToTaskWindow(QString(), ProjectExplorer::BuildParserInterface::Error, -1, lne);
    }
}
