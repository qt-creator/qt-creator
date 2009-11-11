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

#include "msvcparser.h"
#include "projectexplorerconstants.h"

#include <QtCore/QStringList>
#include <QtCore/QDir>

using namespace ProjectExplorer;

MsvcParser::MsvcParser()
{
    m_compileRegExp.setPattern("^([^\\(]+)\\((\\d+)\\)+\\s:[^:\\d]+(\\d+):(.*)$");
    m_compileRegExp.setMinimal(true);
    m_linkRegExp.setPattern("^([^\\(]+)\\s:[^:\\d]+(\\d+):(.*)$");
    m_linkRegExp.setMinimal(true);
}

QString MsvcParser::name() const
{
    return QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_MSVC);
}

void MsvcParser::stdError(const QString & line)
{
    Q_UNUSED(line)
    //do nothing
}

void MsvcParser::stdOutput(const QString & line)
{
    QString lne = line.trimmed();
    if (m_compileRegExp.indexIn(lne) > -1 && m_compileRegExp.numCaptures() == 4) {
        emit addToTaskWindow(
            QDir::cleanPath(m_compileRegExp.cap(1)), //filename
            toType(m_compileRegExp.cap(3).toInt()), // PatternType
            m_compileRegExp.cap(2).toInt(), //linenumber
            m_compileRegExp.cap(4) //description
            );

    } else if (m_linkRegExp.indexIn(lne) > -1 && m_linkRegExp.numCaptures() == 3) {
        QString fileName = m_linkRegExp.cap(1);
        if (fileName.contains(QLatin1String("LINK"), Qt::CaseSensitive))
            fileName.clear();

        emit addToTaskWindow(
            QDir::cleanPath(fileName), //filename
            toType(m_linkRegExp.cap(2).toInt()), // pattern type
            -1, // line number
            m_linkRegExp.cap(3) // description
            );
    }
}

TaskWindow::TaskType MsvcParser::toType(int number)
{
    if (number == 0)
        return TaskWindow::Unknown;
    else if (number > 4000 && number < 5000)
        return TaskWindow::Warning;
    else
        return TaskWindow::Error;
}
