/****************************************************************************
**
** Copyright (C) 2015 Jan Dalheimer <jan@dalheimer.de>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakeindenter.h"

#include <QStack>
#include <QDebug>

#include <texteditor/tabsettings.h>
#include <texteditor/textdocumentlayout.h>

namespace CMakeProjectManager {
namespace Internal {

CMakeIndenter::CMakeIndenter()
{
}

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

void CMakeIndenter::indentBlock(QTextDocument *doc, const QTextBlock &block, const QChar &typedChar, const TextEditor::TabSettings &tabSettings)
{
    Q_UNUSED(doc)
    Q_UNUSED(typedChar)

    QTextBlock previousBlock = block.previous();
    // find the next previous block that is non-empty (contains non-whitespace characters)
    while (previousBlock.isValid() && lineIsEmpty(previousBlock.text()))
        previousBlock = previousBlock.previous();
    if (previousBlock.isValid()) {
        const QString previousLine = previousBlock.text();
        const QString currentLine = block.text();
        int indentation = tabSettings.indentationColumn(previousLine);

        if (lineStartsBlock(previousLine))
            indentation += tabSettings.m_indentSize;
        if (lineEndsBlock(currentLine))
            indentation = qMax(0, indentation - tabSettings.m_indentSize);

        // increase/decrease/keep the indentation level depending on if we have more opening or closing parantheses
        indentation = qMax(0, indentation + tabSettings.m_indentSize * paranthesesLevel(previousLine));

        tabSettings.indentLine(block, indentation);
    } else {
        // First line in whole document
        tabSettings.indentLine(block, 0);
    }
}

} // namespace Internal
} // namespace CMakeProjectManager
