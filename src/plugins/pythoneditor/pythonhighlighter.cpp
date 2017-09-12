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

/**
 * @brief The Highlighter class pre-highlights Python source using simple scanner.
 *
 * Highlighter doesn't highlight user types (classes and enumerations), syntax
 * and semantic errors, unnecessary code, etc. It's implements only
 * basic highlight mechanism.
 *
 * Main highlight procedure is highlightBlock().
 */

#include "pythonhighlighter.h"
#include "pythonscanner.h"

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>

namespace PythonEditor {
namespace Internal {

/**
 * @class PythonEditor::Internal::PythonHighlighter
 * @brief Handles incremental lexical highlighting, but not semantic
 *
 * Incremental lexical highlighting works every time when any character typed
 * or some text inserted (i.e. copied & pasted).
 * Each line keeps associated scanner state - integer number. This state is the
 * scanner context for next line. For example, 3 quotes begin a multiline
 * string, and each line up to next 3 quotes has state 'MultiLineString'.
 *
 * @code
 *  def __init__:               # Normal
 *      self.__doc__ = """      # MultiLineString (next line is inside)
 *                     banana   # MultiLineString
 *                     """      # Normal
 * @endcode
 */

static TextEditor::TextStyle styleForFormat(int format)
{
    using namespace TextEditor;
    const auto f = Format(format);
    switch (f) {
    case Format_Number: return C_NUMBER;
    case Format_String: return C_STRING;
    case Format_Keyword: return C_KEYWORD;
    case Format_Type: return C_TYPE;
    case Format_ClassField: return C_FIELD;
    case Format_MagicAttr: return C_JS_SCOPE_VAR;
    case Format_Operator: return C_OPERATOR;
    case Format_Comment: return C_COMMENT;
    case Format_Doxygen: return C_DOXYGEN_COMMENT;
    case Format_Identifier: return C_TEXT;
    case Format_Whitespace: return C_VISUAL_WHITESPACE;
    case Format_ImportedModule: return C_STRING;
    case Format_FormatsAmount:
        QTC_CHECK(false); // should never get here
        return C_TEXT;
    }
    QTC_CHECK(false); // should never get here
    return C_TEXT;
}

PythonHighlighter::PythonHighlighter()
{
    setTextFormatCategories(Format_FormatsAmount, styleForFormat);
}

/**
 * @brief PythonHighlighter::highlightBlock highlights single line of Python code
 * @param text is single line without EOLN symbol. Access to all block data
 * can be obtained through inherited currentBlock() function.
 *
 * This function receives state (int number) from previously highlighted block,
 * scans block using received state and sets initial highlighting for current
 * block. At the end, it saves internal state in current block.
 */
void PythonHighlighter::highlightBlock(const QString &text)
{
    int initialState = previousBlockState();
    if (initialState == -1)
        initialState = 0;
    setCurrentBlockState(highlightLine(text, initialState));
}

/**
 * @return True if this keyword is acceptable at start of import line
 */
static bool isImportKeyword(const QString &keyword)
{
    return keyword == "import" || keyword == "from";
}

static int indent(const QString &line)
{
    for (int i = 0, size = line.size(); i < size; ++i) {
        if (!line.at(i).isSpace())
            return i;
    }
    return -1;
}

static void setFoldingIndent(const QTextBlock &block, int indent)
{
    if (TextEditor::TextBlockUserData *userData = TextEditor::TextDocumentLayout::userData(block)) {
         userData->setFoldingIndent(indent);
         userData->setFoldingStartIncluded(false);
         userData->setFoldingEndIncluded(false);
    }
}

/**
 * @brief Highlight line of code, returns new block state
 * @param text Source code to highlight
 * @param initialState Initial state of scanner, retrieved from previous block
 * @return Final state of scanner, should be saved with current block
 */
int PythonHighlighter::highlightLine(const QString &text, int initialState)
{
    Scanner scanner(text.constData(), text.size());
    scanner.setState(initialState);

    const int pos = indent(text);
    if (pos < 0) {
        // Empty lines do not change folding indent
        setFoldingIndent(currentBlock(), m_lastIndent);
    } else {
        m_lastIndent = pos;
        if (pos == 0 && text.startsWith('#') && !text.startsWith("#!")) {
            // A comment block at indentation 0. Fold on first line.
            setFoldingIndent(currentBlock(), withinLicenseHeader ? 1 : 0);
            withinLicenseHeader = true;
        } else {
            // Normal Python code. Line indentation can be used as folding indent.
            setFoldingIndent(currentBlock(), m_lastIndent);
            withinLicenseHeader = false;
        }
    }

    FormatToken tk;
    bool hasOnlyWhitespace = true;
    while (!(tk = scanner.read()).isEndOfBlock()) {
        Format format = tk.format();
        if (format == Format_Keyword && isImportKeyword(scanner.value(tk)) && hasOnlyWhitespace) {
            setFormat(tk.begin(), tk.length(), formatForCategory(format));
            highlightImport(scanner);
        } else if (format == Format_Comment
                   || format == Format_String
                   || format == Format_Doxygen) {
            setFormatWithSpaces(text, tk.begin(), tk.length(), formatForCategory(format));
        } else {
            setFormat(tk.begin(), tk.length(), formatForCategory(format));
        }
        if (format != Format_Whitespace)
            hasOnlyWhitespace = false;
    }
    return scanner.state();
}

/**
 * @brief Highlights rest of line as import directive
 */
void PythonHighlighter::highlightImport(Scanner &scanner)
{
    FormatToken tk;
    while (!(tk = scanner.read()).isEndOfBlock()) {
        Format format = tk.format();
        if (tk.format() == Format_Identifier)
            format = Format_ImportedModule;
        setFormat(tk.begin(), tk.length(), formatForCategory(format));
    }
}

} // namespace Internal
} // namespace PythonEditor
