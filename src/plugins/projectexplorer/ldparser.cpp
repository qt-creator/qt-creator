/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "ldparser.h"
#include "projectexplorerconstants.h"
#include "task.h"

using namespace ProjectExplorer;

namespace {
    // opt. drive letter + filename: (2 brackets)
    const char * const FILE_PATTERN = "(([A-Za-z]:)?[^:]+\\.[^:]+):";
    // line no. or elf segment + offset (1 bracket)
    // const char * const POSITION_PATTERN = "(\\d+|\\(\\.[^:]+[+-]0x[a-fA-F0-9]+\\):)";
    const char * const POSITION_PATTERN = "(\\d|\\(\\..+[+-]0x[a-fA-F0-9]+\\):)";
    const char * const COMMAND_PATTERN = "^(.*[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(ld|gold)(-[0-9\\.]+)?(\\.exe)?: ";
}

LdParser::LdParser()
{
    setObjectName(QLatin1String("LdParser"));
    m_regExpLinker.setPattern(QLatin1Char('^') +
                              QString::fromLatin1(FILE_PATTERN) + QLatin1Char('(') +
                              QString::fromLatin1(FILE_PATTERN) + QLatin1String(")?(") +
                              QLatin1String(POSITION_PATTERN) + QLatin1String(")?\\s(.+)$"));
    m_regExpLinker.setMinimal(true);

    m_regExpGccNames.setPattern(QLatin1String(COMMAND_PATTERN));
    m_regExpGccNames.setMinimal(true);
}

void LdParser::stdError(const QString &line)
{
    QString lne = line.trimmed();
    if (lne.startsWith(QLatin1String("TeamBuilder "))
            || lne.startsWith(QLatin1String("distcc["))
            || lne.contains(QLatin1String("ar: creating "))) {
        IOutputParser::stdError(line);
        return;
    }

    if (lne.startsWith(QLatin1String("collect2:"))) {
        emit addTask(Task(Task::Error,
                          lne /* description */,
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(Constants::TASK_CATEGORY_COMPILE)));
        return;
    } else if (m_regExpGccNames.indexIn(lne) > -1) {
        QString description = lne.mid(m_regExpGccNames.matchedLength());
        Task task(Task::Error,
                  description,
                  Utils::FileName(), /* filename */
                  -1, /* line */
                  Core::Id(Constants::TASK_CATEGORY_COMPILE));
        if (description.startsWith(QLatin1String("warning: "))) {
            task.type = Task::Warning;
            task.description = description.mid(9);
        } else if (description.startsWith(QLatin1String("fatal: ")))  {
            task.description = description.mid(7);
        }
        emit addTask(task);
        return;
    } else if (m_regExpLinker.indexIn(lne) > -1) {
        bool ok;
        int lineno = m_regExpLinker.cap(7).toInt(&ok);
        if (!ok)
            lineno = -1;
        Utils::FileName filename = Utils::FileName::fromUserInput(m_regExpLinker.cap(1));
        if (!m_regExpLinker.cap(4).isEmpty()
            && !m_regExpLinker.cap(4).startsWith(QLatin1String("(.text")))
            filename = Utils::FileName::fromUserInput(m_regExpLinker.cap(4));
        QString description = m_regExpLinker.cap(8).trimmed();
        Task task(Task::Error, description, filename, lineno,
                  Core::Id(Constants::TASK_CATEGORY_COMPILE));
        if (description.startsWith(QLatin1String("At global scope")) ||
            description.startsWith(QLatin1String("At top level")) ||
            description.startsWith(QLatin1String("instantiated from ")) ||
            description.startsWith(QLatin1String("In ")))
            task.type = Task::Unknown;
        if (description.startsWith(QLatin1String("warning: "), Qt::CaseInsensitive)) {
            task.type = Task::Warning;
            task.description = description.mid(9);
        }

        emit addTask(task);
        return;
    }

    IOutputParser::stdError(line);
}
