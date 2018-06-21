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

#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include "githighlighters.h"

namespace Git {
namespace Internal {

static const char CHANGE_PATTERN[] = "\\b[a-f0-9]{7,40}\\b";

GitSubmitHighlighter::GitSubmitHighlighter(QTextEdit * parent) :
    TextEditor::SyntaxHighlighter(parent)
{
    setDefaultTextFormatCategories();
    m_keywordPattern.setPattern("^[\\w-]+:");
    m_hashChar = '#';
    QTC_CHECK(m_keywordPattern.isValid());
}

void GitSubmitHighlighter::highlightBlock(const QString &text)
{
    // figure out current state
    State state = static_cast<State>(previousBlockState());
    if (text.trimmed().isEmpty()) {
        if (state == Header)
            state = Other;
        setCurrentBlockState(state);
        return;
    } else if (text.startsWith(m_hashChar)) {
        setFormat(0, text.size(), formatForCategory(TextEditor::C_COMMENT));
        setCurrentBlockState(state);
        return;
    } else if (state == None) {
        state = Header;
    }

    setCurrentBlockState(state);
    // Apply format.
    switch (state) {
    case None:
        break;
    case Header: {
        QTextCharFormat charFormat = format(0);
        charFormat.setFontWeight(QFont::Bold);
        setFormat(0, text.size(), charFormat);
        break;
    }
    case Other:
        // Format key words ("Task:") italic
        if (m_keywordPattern.indexIn(text, 0, QRegExp::CaretAtZero) == 0) {
            QTextCharFormat charFormat = format(0);
            charFormat.setFontItalic(true);
            setFormat(0, m_keywordPattern.matchedLength(), charFormat);
        }
        break;
    }
}

GitRebaseHighlighter::RebaseAction::RebaseAction(const QString &regexp,
                                                 const Format formatCategory)
    : exp(regexp),
      formatCategory(formatCategory)
{
}

static TextEditor::TextStyle styleForFormat(int format)
{
    using namespace TextEditor;
    const auto f = Format(format);
    switch (f) {
    case Format_Comment: return C_COMMENT;
    case Format_Change: return C_DOXYGEN_COMMENT;
    case Format_Description: return C_STRING;
    case Format_Pick: return C_KEYWORD;
    case Format_Reword: return C_FIELD;
    case Format_Edit: return C_TYPE;
    case Format_Squash: return C_ENUMERATION;
    case Format_Fixup: return C_NUMBER;
    case Format_Exec: return C_LABEL;
    case Format_Count:
        QTC_CHECK(false); // should never get here
        return C_TEXT;
    }
    QTC_CHECK(false); // should never get here
    return C_TEXT;
}

GitRebaseHighlighter::GitRebaseHighlighter(QTextDocument *parent) :
    TextEditor::SyntaxHighlighter(parent),
    m_hashChar('#'),
    m_changeNumberPattern(CHANGE_PATTERN)
{
    setTextFormatCategories(Format_Count, styleForFormat);

    m_actions << RebaseAction("^(p|pick)\\b", Format_Pick);
    m_actions << RebaseAction("^(r|reword)\\b", Format_Reword);
    m_actions << RebaseAction("^(e|edit)\\b", Format_Edit);
    m_actions << RebaseAction("^(s|squash)\\b", Format_Squash);
    m_actions << RebaseAction("^(f|fixup)\\b", Format_Fixup);
    m_actions << RebaseAction("^(x|exec)\\b", Format_Exec);
}

void GitRebaseHighlighter::highlightBlock(const QString &text)
{
    if (text.startsWith(m_hashChar)) {
        setFormat(0, text.size(), formatForCategory(Format_Comment));
        int changeIndex = 0;
        while ((changeIndex = m_changeNumberPattern.indexIn(text, changeIndex)) != -1) {
            const int changeLen = m_changeNumberPattern.matchedLength();
            setFormat(changeIndex, changeLen, formatForCategory(Format_Change));
            changeIndex += changeLen;
        }
    } else {
        for (const RebaseAction &action : qAsConst(m_actions)) {
            if (action.exp.indexIn(text) != -1) {
                const int len = action.exp.matchedLength();
                setFormat(0, len, formatForCategory(action.formatCategory));
                const int changeIndex = m_changeNumberPattern.indexIn(text, len);
                if (changeIndex != -1) {
                    const int changeLen = m_changeNumberPattern.matchedLength();
                    const int descStart = changeIndex + changeLen + 1;
                    setFormat(changeIndex, changeLen, formatForCategory(Format_Change));
                    setFormat(descStart, text.size() - descStart, formatForCategory(Format_Description));
                }
                break;
            }
        }
    }
    formatSpaces(text);
}

} // namespace Internal
} // namespace Git
