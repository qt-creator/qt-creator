/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "gccparser.h"
#include "qt4projectmanagerconstants.h"

#include <QDebug>

using namespace Qt4ProjectManager;

GccParser::GccParser()
{
    m_regExp.setPattern("^([^\\(\\)]+[^\\d]):(\\d+):(\\d+:)*(\\s(warning|error):)?(.+)$");
    m_regExp.setMinimal(true);

    m_regExpIncluded.setPattern("^.*from\\s([^:]+):(\\d+)(,|:)$");
    m_regExpIncluded.setMinimal(true);

    m_regExpLinker.setPattern("^(\\S+)\\(\\S+\\):(.+)$");
    m_regExpLinker.setMinimal(true);

    //make[4]: Entering directory `/home/kkoehne/dev/ide-explorer/src/plugins/qtscripteditor'
    m_makeDir.setPattern("^make.*: (\\w+) directory .(.+).$");
    m_makeDir.setMinimal(true);

    m_linkIndent = false;
}

QString GccParser::name() const
{
    return QLatin1String(Qt4ProjectManager::Constants::BUILD_PARSER_GCC);
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
    if (m_regExp.indexIn(lne) > -1) {
        ProjectExplorer::BuildParserInterface::PatternType type;
        if (m_regExp.cap(5) == "warning")
            type = ProjectExplorer::BuildParserInterface::Warning;
        else if (m_regExp.cap(5) == "error")
            type = ProjectExplorer::BuildParserInterface::Error;
        else
            type = ProjectExplorer::BuildParserInterface::Unknown;

        QString description =  m_regExp.cap(6);
        if (m_linkIndent)
            description.prepend(QLatin1String("-> "));

        //qDebug()<<m_regExp.cap(1)<<m_regExp.cap(2)<<m_regExp.cap(3)<<m_regExp.cap(4);

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
    } else if (m_regExpLinker.indexIn(lne) > -1) {
        ProjectExplorer::BuildParserInterface::PatternType type = ProjectExplorer::BuildParserInterface::Error;
        QString description = m_regExpLinker.cap(2);
        if (lne.endsWith(QLatin1Char(':'))) {
            m_linkIndent = true;
        } else if (m_linkIndent) {
            description.prepend(QLatin1String("-> "));
            type = ProjectExplorer::BuildParserInterface::Unknown;
        }
        emit addToTaskWindow(
            m_regExpLinker.cap(1), //filename
            type,
            -1, //linenumber
            description);
    } else if (lne.startsWith(QLatin1String("collect2:"))) {
        emit addToTaskWindow(
            "",
            ProjectExplorer::BuildParserInterface::Error,
            -1,
            lne //description
            );
        m_linkIndent = false;
    } else {
        m_linkIndent = false;
    }
}
