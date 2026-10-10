// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include "gitconstants.h"
#include "githighlighters.h"

#include <QTextBlock>

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

    // The whole of a submit message is prose, bar the comments the editor strips.
    addProseRange(0, text.size());
    spellCheck(text);
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

GitLogHighlighter::GitLogHighlighter()
    : VcsBase::DiffAndLogHighlighter(
          QRegularExpression("^(?:diff --git a/|index |[+-]{3} (?:/dev/null|[ab]/(.+$)))"),
          QRegularExpression("^commit ([0-9a-f]{8})[0-9a-f]{32}"))
{
}

void GitLogHighlighter::highlightBlock(const QString &text)
{
    VcsBase::DiffAndLogHighlighter::highlightBlock(text);

    static const QRegularExpression prefixPattern(R"(^[ |*\/\\]*)");
    static const QRegularExpression graphPrefixPattern(R"(^[ |*\/\\]*[|*\/\\] ?)");
    static const QRegularExpression commitPattern(R"(^commit [a-f0-9]{7,40}\b)");
    static const QRegularExpression trailerPattern(R"(^((?:[\w]+-)+[\w]+|Fixes):)");
    static const QRegularExpression amendsPattern(R"(^(Amends) [a-f0-9]{7,40}\.?\s*$)");
    static const QRegularExpression cherryPickPattern(
        R"(^\((cherry picked) from commit [a-f0-9]{7,40}\)\s*$)");
    const auto contentOf = [](const QString &line) {
        return line.mid(prefixPattern.match(line).capturedLength());
    };
    const auto isFooterLine = [](const QString &line) {
        return trailerPattern.match(line).hasMatch() || amendsPattern.match(line).hasMatch()
               || cherryPickPattern.match(line).hasMatch();
    };
    const auto boundaryTextOf = [](const QString &line) {
        return line.mid(graphPrefixPattern.match(line).capturedLength());
    };
    const int prefixLength = prefixPattern.match(text).capturedLength();
    const QString content = text.mid(prefixLength);

    enum State { Header, BeforeSubject, Subject, Body, Diff };
    int state = previousBlockState();
    const QString boundaryText = boundaryTextOf(text);
    if (state == -1 || commitPattern.match(boundaryText).hasMatch())
        state = Header;
    else if (boundaryText.startsWith("diff --git "))
        state = Diff;
    else if (state == Header && content.trimmed().isEmpty())
        state = BeforeSubject;
    else if (state == BeforeSubject && !content.trimmed().isEmpty())
        state = Subject;
    else if (state == Subject && content.trimmed().isEmpty())
        state = Body;
    setCurrentBlockState(state);
    if (state != Body || !isFooterLine(content))
        return;

    for (QTextBlock block = currentBlock().previous(); block.isValid(); block = block.previous()) {
        const QString line = contentOf(block.text());
        if (line.trimmed().isEmpty())
            break;
        if (!isFooterLine(line))
            return;
    }
    for (QTextBlock block = currentBlock().next(); block.isValid(); block = block.next()) {
        const QString boundaryText = boundaryTextOf(block.text());
        if (commitPattern.match(boundaryText).hasMatch() || boundaryText.startsWith("diff --git "))
            break;
        const QString line = contentOf(block.text());
        if (!line.trimmed().isEmpty() && !isFooterLine(line))
            return;
    }

    const QRegularExpressionMatch trailer = trailerPattern.match(content);
    if (trailer.hasMatch()) {
        setFormat(prefixLength, trailer.capturedLength(),
                  formatForCategory(TextEditor::C_LOG_COMMIT_FOOTER));
    } else {
        QRegularExpressionMatch reference = amendsPattern.match(content);
        if (!reference.hasMatch())
            reference = cherryPickPattern.match(content);
        if (!reference.hasMatch())
            return;
        setFormat(prefixLength + reference.capturedStart(1), reference.capturedLength(1),
                  formatForCategory(TextEditor::C_LOG_COMMIT_FOOTER));
    }
}

GitReflogHighlighter::GitReflogHighlighter()
    : VcsBase::DiffAndLogHighlighter(QRegularExpression("(?!)"), entryPattern())
{
}

const QRegularExpression &GitReflogHighlighter::entryPattern()
{
    static const QRegularExpression pattern("^([0-9a-f]{7,40}) [^}]*\\}: .*$");
    return pattern;
}

void GitReflogHighlighter::highlightBlock(const QString &text)
{
    VcsBase::DiffAndLogHighlighter::highlightBlock(text);

    const QRegularExpressionMatch entryMatch = entryPattern().match(text);
    if (!entryMatch.hasMatch())
        return;

    // DiffAndLogHighlighter colors the complete log-entry line. Reflog
    // entries are more useful when only their individual fields are colored.
    setFormatWithSpaces(text, 0, text.size(), formatForCategory(TextEditor::C_TEXT));
    setFormat(entryMatch.capturedStart(1), entryMatch.capturedLength(1),
              formatForCategory(TextEditor::C_LOG_COMMIT_HASH));

    const QRegularExpression selectorPattern(R"(\S+@\{[^}]*\})");
    const QRegularExpressionMatch selectorMatch = selectorPattern.match(
        text, entryMatch.capturedEnd(1));
    if (!selectorMatch.hasMatch())
        return;

    const QRegularExpression decorationPattern(R"(\([^)]*\))");
    const QRegularExpressionMatch decorationMatch = decorationPattern.match(
        text, entryMatch.capturedEnd(1));
    if (decorationMatch.hasMatch()
        && decorationMatch.capturedEnd() <= selectorMatch.capturedStart()) {
        setFormat(decorationMatch.capturedStart(), decorationMatch.capturedLength(),
                  formatForCategory(TextEditor::C_LOG_DECORATION));
    }

    setFormat(selectorMatch.capturedStart(), selectorMatch.capturedLength(),
              formatForCategory(TextEditor::C_LOG_COMMIT_DATE));
}

} // Git::Internal
