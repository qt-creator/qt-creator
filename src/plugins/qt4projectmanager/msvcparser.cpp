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
***************************************************************************/
#include <QtCore/QStringList>

#include "msvcparser.h"
#include "qt4projectmanagerconstants.h"

using namespace Qt4ProjectManager;

MsvcParser::MsvcParser()
{
    m_compileRegExp.setPattern("^([^\\(]+)\\((\\d+)\\)+\\s:[^:\\d]+(\\d+):(.*)$");
    m_compileRegExp.setMinimal(true);
    m_linkRegExp.setPattern("^([^\\(]+)\\s:[^:\\d]+(\\d+):(.*)$");
    m_linkRegExp.setMinimal(true);
}

QString MsvcParser::name() const
{
    return QLatin1String(Qt4ProjectManager::Constants::BUILD_PARSER_MSVC);
}

void MsvcParser::stdError(const QString & line)
{
    Q_UNUSED(line);
    //do nothing
}

void MsvcParser::stdOutput(const QString & line)
{
    QString lne = line.trimmed();
    if (m_compileRegExp.indexIn(lne) > -1 && m_compileRegExp.numCaptures() == 4) {
        emit addToTaskWindow(
            m_compileRegExp.cap(1), //filename
            toType(m_compileRegExp.cap(3).toInt()), // PatternType
            m_compileRegExp.cap(2).toInt(), //linenumber
            m_compileRegExp.cap(4) //description
            );

    } else if (m_linkRegExp.indexIn(lne) > -1 && m_linkRegExp.numCaptures() == 3) {
        QString fileName = m_linkRegExp.cap(1);
        if (fileName.contains(QLatin1String("LINK"), Qt::CaseSensitive))
            fileName.clear();

        emit addToTaskWindow(
            fileName, //filename
            toType(m_linkRegExp.cap(2).toInt()), // pattern type
            -1, // line number
            m_linkRegExp.cap(3) // description
            );
    }
}

ProjectExplorer::BuildParserInterface::PatternType MsvcParser::toType(int number)
{
    if (number == 0)
        return ProjectExplorer::BuildParserInterface::Unknown;
    else if (number > 4000 && number < 5000)
        return ProjectExplorer::BuildParserInterface::Warning;
    else
        return ProjectExplorer::BuildParserInterface::Error;
}
