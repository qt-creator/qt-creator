// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include "gitconstants.h"
#include "githighlighters.h"

namespace Git::Internal {

const char CHANGE_PATTERN[] = "\\b[a-f0-9]{7,40}\\b";

GitSubmitHighlighter::GitSubmitHighlighter(QChar commentChar, QTextEdit * parent) :
    TextEditor::SyntaxHighlighter(parent),
    m_keywordPattern("^[\\w-]+:")
{
    setDefaultTextFormatCategories();
    m_commentChar = commentChar.isNull() ? QChar(Constants::DEFAULT_COMMENT_CHAR) : commentChar;
    QTC_CHECK(m_keywordPattern.isValid());
}

void GitSubmitHighlighter::highlightBlock(const QString &text)
{
    // figure out current state
    auto state = static_cast<State>(previousBlockState());
    if (text.trimmed().isEmpty()) {
        if (state == Header)
            state = Other;
        setCurrentBlockState(state);
        return;
    } else if (text.startsWith(m_commentChar)) {
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
        const QRegularExpressionMatch match = m_keywordPattern.match(text);
        if (match.hasMatch() && match.capturedStart(0) == 0) {
            QTextCharFormat charFormat = format(0);
            charFormat.setFontItalic(true);
            setFormat(0, match.capturedLength(), charFormat);
        }
        break;
    }
}

QChar GitSubmitHighlighter::commentChar() const
{
    return m_commentChar;
}

void GitSubmitHighlighter::setCommentChar(QChar commentChar)
{
    if (m_commentChar == commentChar)
        return;
    m_commentChar = commentChar;
    rehighlight();
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
    case Format_Break: return C_PREPROCESSOR;
    case Format_Drop: return C_REMOVED_LINE;
    case Format_Label: return C_LABEL;
    case Format_Reset: return C_LABEL;
    case Format_Merge: return C_LABEL;
    case Format_Count:
        QTC_CHECK(false); // should never get here
        return C_TEXT;
    }
    QTC_CHECK(false); // should never get here
    return C_TEXT;
}

GitRebaseHighlighter::GitRebaseHighlighter(QChar commentChar, QTextDocument *parent) :
    TextEditor::SyntaxHighlighter(parent),
    m_commentChar(commentChar),
    m_changeNumberPattern(CHANGE_PATTERN)
{
    setTextFormatCategories(Format_Count, styleForFormat);

    m_actions << RebaseAction("^(p|pick)\\b", Format_Pick);
    m_actions << RebaseAction("^(r|reword)\\b", Format_Reword);
    m_actions << RebaseAction("^(e|edit)\\b", Format_Edit);
    m_actions << RebaseAction("^(s|squash)\\b", Format_Squash);
    m_actions << RebaseAction("^(f|fixup)\\b", Format_Fixup);
    m_actions << RebaseAction("^(x|exec)\\b", Format_Exec);
    m_actions << RebaseAction("^(b|break)\\b", Format_Break);
    m_actions << RebaseAction("^(d|drop)\\b", Format_Drop);
    m_actions << RebaseAction("^(l|label)\\b", Format_Label);
    m_actions << RebaseAction("^(t|reset)\\b", Format_Reset);
    m_actions << RebaseAction("^(m|merge)\\b", Format_Merge);
}

void GitRebaseHighlighter::highlightBlock(const QString &text)
{
    if (text.startsWith(m_commentChar)) {
        setFormat(0, text.size(), formatForCategory(Format_Comment));
        QRegularExpressionMatchIterator it = m_changeNumberPattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), formatForCategory(Format_Change));
        }
    } else {
        for (const RebaseAction &action : std::as_const(m_actions)) {
            const QRegularExpressionMatch match = action.exp.match(text);
            if (match.hasMatch()) {
                const int len = match.capturedLength();
                setFormat(0, len, formatForCategory(action.formatCategory));
                const QRegularExpressionMatch changeMatch = m_changeNumberPattern.match(text, len);
                const int changeIndex = changeMatch.capturedStart();
                if (changeMatch.hasMatch()) {
                    const int changeLen = changeMatch.capturedLength();
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

} // Git::Internal
