// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include "gitconstants.h"
#include "githighlighters.h"

namespace Git::Internal {

const char CHANGE_PATTERN[] = "\\b[a-f0-9]{7,40}\\b";

GitSubmitHighlighter::GitSubmitHighlighter(const QString &commentMarker, QTextEdit *parent) :
    TextEditor::SyntaxHighlighter(parent),
    m_keywordPattern("^[\\w-]+:")
{
    setDefaultTextFormatCategories();
    m_commentMarker =
        commentMarker.isNull() ? QString(Constants::DEFAULT_COMMENT_CHAR) : commentMarker;
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
    } else if (text.startsWith(m_commentMarker)) {
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

QString GitSubmitHighlighter::commentMarker() const
{
    return m_commentMarker;
}

void GitSubmitHighlighter::setCommentChar(const QString &commentMarker)
{
    if (m_commentMarker == commentMarker)
        return;
    m_commentMarker = commentMarker;
    rehighlight();
}

GitRebaseHighlighter::RebaseAction::RebaseAction(QChar shortcut, const QString &action,
                                                 const Format formatCategory)
    : shortcut(shortcut),
      action(action),
      exp("^(" + QRegularExpression::escape(QString(shortcut)) + '|' + action + ")\\b"),
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
    case Format_UpdateRef: return C_KEYWORD;
    case Format_Count:
        QTC_CHECK(false); // should never get here
        return C_TEXT;
    }
    QTC_CHECK(false); // should never get here
    return C_TEXT;
}

GitRebaseHighlighter::GitRebaseHighlighter(const QString &commentMarker, QTextDocument *parent) :
    TextEditor::SyntaxHighlighter(parent),
    m_commentMarker(commentMarker),
    m_changeNumberPattern(CHANGE_PATTERN)
{
    setTextFormatCategories(Format_Count, styleForFormat);
}

const QList<GitRebaseHighlighter::RebaseAction> &GitRebaseHighlighter::actions()
{
    static const QList<RebaseAction> actions {
        RebaseAction('p', "pick", Format_Pick),
        RebaseAction('r', "reword", Format_Reword),
        RebaseAction('e', "edit", Format_Edit),
        RebaseAction('s', "squash", Format_Squash),
        RebaseAction('f', "fixup", Format_Fixup),
        RebaseAction('x', "exec", Format_Exec),
        RebaseAction('b', "break", Format_Break),
        RebaseAction('d', "drop", Format_Drop),
        RebaseAction('l', "label", Format_Label),
        RebaseAction('t', "reset", Format_Reset),
        RebaseAction('m', "merge", Format_Merge),
        RebaseAction('u', "update-ref", Format_UpdateRef),
    };
    return actions;
}

void GitRebaseHighlighter::highlightBlock(const QString &text)
{
    if (text.startsWith(m_commentMarker)) {
        setFormat(0, text.size(), formatForCategory(Format_Comment));
        QRegularExpressionMatchIterator it = m_changeNumberPattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), formatForCategory(Format_Change));
        }
    } else {
        for (const RebaseAction &action : actions()) {
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
