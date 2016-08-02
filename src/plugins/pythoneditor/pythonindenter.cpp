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

#include "pythonindenter.h"
#include "pythonscanner.h"

#include <texteditor/tabsettings.h>

namespace PythonEditor {

/**
 * @brief Does given character change indentation level?
 * @param ch Any value
 * @return True if character increases indentation level at the next line
 */
bool PythonIndenter::isElectricCharacter(const QChar &ch) const
{
    return ch == ':';
}

int PythonIndenter::indentFor(const QTextBlock &block, const TextEditor::TabSettings &tabSettings)
{
    QTextBlock previousBlock = block.previous();
    if (!previousBlock.isValid())
        return 0;

    QString previousLine = previousBlock.text();
    int indentation = tabSettings.indentationColumn(previousLine);

    if (isElectricLine(previousLine))
        indentation += tabSettings.m_indentSize;
    else
        indentation = qMax<int>(0, indentation + getIndentDiff(previousLine, tabSettings));

    return indentation;
}

/// @return True if electric character is last non-space character at given string
bool PythonIndenter::isElectricLine(const QString &line) const
{
    if (line.isEmpty())
        return false;

    // trim spaces in 'if True:  '
    int index = line.length() - 1;
    while (index > 0 && line[index].isSpace())
        --index;

    return isElectricCharacter(line[index]);
}

/// @return negative indent diff if previous line breaks control flow branch
int PythonIndenter::getIndentDiff(const QString &previousLine,
                                  const TextEditor::TabSettings &tabSettings) const
{
    static const QStringList jumpKeywords = {
        "return", "yield", "break", "continue", "raise", "pass" };

    Internal::Scanner sc(previousLine.constData(), previousLine.length());
    forever {
        Internal::FormatToken tk = sc.read();
        if (tk.format() == Internal::Format_Keyword && jumpKeywords.contains(sc.value(tk)))
            return -tabSettings.m_indentSize;
        if (tk.format() != Internal::Format_Whitespace)
            break;
    }
    return 0;
}

} // namespace PythonEditor
