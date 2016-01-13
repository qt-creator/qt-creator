/****************************************************************************
**
** Copyright (C) 2016 Jan Dalheimer <jan@dalheimer.de>
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

#include "cmakeindenter.h"

#include <QStack>
#include <QDebug>

#include <texteditor/tabsettings.h>
#include <texteditor/textdocumentlayout.h>

namespace CMakeProjectManager {
namespace Internal {

bool CMakeIndenter::isElectricCharacter(const QChar &ch) const
{
    return ch == QLatin1Char('(') || ch == QLatin1Char(')');
}

static bool lineContainsFunction(const QString &line, const QString &function)
{
    const int indexOfFunction = line.indexOf(function);
    if (indexOfFunction == -1)
        return false;
    for (int i = 0; i < indexOfFunction; ++i) {
        if (!line.at(i).isSpace())
            return false;
    }
    for (int i = indexOfFunction + function.size(); i < line.size(); ++i) {
        if (line.at(i) == QLatin1Char('('))
            return true;
        else if (!line.at(i).isSpace())
            return false;
    }
    return false;
}
static bool lineStartsBlock(const QString &line)
{
    return lineContainsFunction(line, QStringLiteral("function")) ||
            lineContainsFunction(line, QStringLiteral("macro")) ||
            lineContainsFunction(line, QStringLiteral("foreach")) ||
            lineContainsFunction(line, QStringLiteral("while")) ||
            lineContainsFunction(line, QStringLiteral("if")) ||
            lineContainsFunction(line, QStringLiteral("elseif")) ||
            lineContainsFunction(line, QStringLiteral("else"));
}
static bool lineEndsBlock(const QString &line)
{
    return lineContainsFunction(line, QStringLiteral("endfunction")) ||
            lineContainsFunction(line, QStringLiteral("endmacro")) ||
            lineContainsFunction(line, QStringLiteral("endforeach")) ||
            lineContainsFunction(line, QStringLiteral("endwhile")) ||
            lineContainsFunction(line, QStringLiteral("endif")) ||
            lineContainsFunction(line, QStringLiteral("elseif")) ||
            lineContainsFunction(line, QStringLiteral("else"));
}
static bool lineIsEmpty(const QString &line)
{
    for (const QChar &c : line) {
        if (!c.isSpace())
            return false;
    }
    return true;
}

static int paranthesesLevel(const QString &line)
{
    const QStringRef beforeComment = line.midRef(0, line.indexOf(QLatin1Char('#')));
    const int opening = beforeComment.count(QLatin1Char('('));
    const int closing = beforeComment.count(QLatin1Char(')'));
    if (opening == closing)
        return 0;
    else if (opening > closing)
        return 1;
    else
        return -1;
}

int CMakeIndenter::indentFor(const QTextBlock &block, const TextEditor::TabSettings &tabSettings)
{
    QTextBlock previousBlock = block.previous();
    // find the next previous block that is non-empty (contains non-whitespace characters)
    while (previousBlock.isValid() && lineIsEmpty(previousBlock.text()))
        previousBlock = previousBlock.previous();
    if (!previousBlock.isValid())
        return 0;

    const QString previousLine = previousBlock.text();
    const QString currentLine = block.text();
    int indentation = tabSettings.indentationColumn(previousLine);

    if (lineStartsBlock(previousLine))
        indentation += tabSettings.m_indentSize;
    if (lineEndsBlock(currentLine))
        indentation = qMax(0, indentation - tabSettings.m_indentSize);

    // increase/decrease/keep the indentation level depending on if we have more opening or closing parantheses
    return qMax(0, indentation + tabSettings.m_indentSize * paranthesesLevel(previousLine));
}

} // namespace Internal
} // namespace CMakeProjectManager


