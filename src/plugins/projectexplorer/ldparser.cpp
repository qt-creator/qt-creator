/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ldparser.h"
#include "projectexplorerconstants.h"
#include "task.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace {
    // opt. drive letter + filename: (2 brackets)
    const char * const FILE_PATTERN = "(([A-Za-z]:)?[^:]+\\.[^:]+):";
    // line no. or elf segment + offset (1 bracket)
    const char * const POSITION_PATTERN = "(\\d+|\\(\\..+?[+-]0x[a-fA-F0-9]+\\)):";
    const char * const COMMAND_PATTERN = "^(.*[\\\\/])?([a-z0-9]+-[a-z0-9]+-[a-z0-9]+-)?(ld|gold)(-[0-9\\.]+)?(\\.exe)?: ";
    const char *const RANLIB_PATTERN = "ranlib(.exe)?: (file: (.*) has no symbols)$";
}

LdParser::LdParser()
{
    setObjectName(QLatin1String("LdParser"));
    m_ranlib.setPattern(QLatin1String(RANLIB_PATTERN));
    QTC_CHECK(m_ranlib.isValid());
    m_regExpLinker.setPattern(QLatin1Char('^') +
                              QString::fromLatin1(FILE_PATTERN) + QLatin1Char('(') +
                              QString::fromLatin1(FILE_PATTERN) + QLatin1String(")?(") +
                              QLatin1String(POSITION_PATTERN) + QLatin1String(")?\\s(.+)$"));
    QTC_CHECK(m_regExpLinker.isValid());

    m_regExpGccNames.setPattern(QLatin1String(COMMAND_PATTERN));
    QTC_CHECK(m_regExpGccNames.isValid());
}

void LdParser::stdError(const QString &line)
{
    QString lne = rightTrimmed(line);
    if (lne.startsWith(QLatin1String("TeamBuilder "))
            || lne.startsWith(QLatin1String("distcc["))
            || lne.contains(QLatin1String("ar: creating "))) {
        IOutputParser::stdError(line);
        return;
    }

    if (lne.startsWith(QLatin1String("collect2:"))) {
        Task task = Task(Task::Error,
                         lne /* description */,
                         Utils::FileName() /* filename */,
                         -1 /* linenumber */,
                         Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task, 1);
        return;
    }

    QRegularExpressionMatch match = m_ranlib.match(lne);
    if (match.hasMatch()) {
        QString description = match.captured(2);
        Task task(Task::Warning, description,
                  Utils::FileName(), -1,
                  Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task, 1);
        return;
    }

    match = m_regExpGccNames.match(lne);
    if (match.hasMatch()) {
        QString description = lne.mid(match.capturedLength());
        Task::TaskType type = Task::Error;
        if (description.startsWith(QLatin1String("warning: "))) {
            type = Task::Warning;
            description = description.mid(9);
        } else if (description.startsWith(QLatin1String("fatal: ")))  {
            description = description.mid(7);
        }
        Task task(type, description, Utils::FileName() /* filename */, -1 /* line */,
                  Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task, 1);
        return;
    }

    match = m_regExpLinker.match(lne);
    if (match.hasMatch()) {
        bool ok;
        int lineno = match.captured(7).toInt(&ok);
        if (!ok)
            lineno = -1;
        Utils::FileName filename = Utils::FileName::fromUserInput(match.captured(1));
        const QString sourceFileName = match.captured(4);
        if (!sourceFileName.isEmpty()
            && !sourceFileName.startsWith(QLatin1String("(.text"))
            && !sourceFileName.startsWith(QLatin1String("(.data"))) {
            filename = Utils::FileName::fromUserInput(sourceFileName);
        }
        QString description = match.captured(8).trimmed();
        Task::TaskType type = Task::Error;
        if (description.startsWith(QLatin1String("At global scope")) ||
            description.startsWith(QLatin1String("At top level")) ||
            description.startsWith(QLatin1String("instantiated from ")) ||
            description.startsWith(QLatin1String("In ")) ||
            description.startsWith(QLatin1String("first defined here")) ||
            description.startsWith(QLatin1String("note:"), Qt::CaseInsensitive)) {
            type = Task::Unknown;
        } else if (description.startsWith(QLatin1String("warning: "), Qt::CaseInsensitive)) {
            type = Task::Warning;
            description = description.mid(9);
        }
        Task task(type, description, filename, lineno, Constants::TASK_CATEGORY_COMPILE);
        emit addTask(task, 1);
        return;
    }

    IOutputParser::stdError(line);
}
