// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonindenter.h"
#include "pythonscanner.h"

#include <texteditor/tabsettings.h>

#include <algorithm>

namespace Python {

static bool isEmptyLine(const QString &t)
{
    return std::all_of(t.cbegin(), t.cend(), [] (QChar c) { return c.isSpace(); });
}

static inline bool isEmptyLine(const QTextBlock &block)
{
    return isEmptyLine(block.text());
}

static QTextBlock previousNonEmptyBlock(const QTextBlock &block)
{
    QTextBlock result = block;
    while (result.isValid() && isEmptyLine(result))
        result = result.previous();
    return result;
}

PythonIndenter::PythonIndenter(QTextDocument *doc)
    : TextEditor::TextIndenter(doc)
{}

/**
 * @brief Does given character change indentation level?
 * @param ch Any value
 * @return True if character increases indentation level at the next line
 */
bool PythonIndenter::isElectricCharacter(const QChar &ch) const
{
    return ch == ':';
}

int PythonIndenter::indentFor(const QTextBlock &block,
                              const TextEditor::TabSettings &tabSettings,
                              int /*cursorPositionInEditor*/)
{
    QTextBlock previousBlock = block.previous();
    if (!previousBlock.isValid())
        return 0;

    // When pasting in actual code, try to skip back past empty lines to an
    // actual code line to find a suitable indentation. This prevents code from
    // not being indented when pasting below an empty line.
    if (!isEmptyLine(block)) {
        const QTextBlock previousNonEmpty = previousNonEmptyBlock(previousBlock);
        if (previousNonEmpty.isValid())
            previousBlock = previousNonEmpty;
    }

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

} // namespace Python
