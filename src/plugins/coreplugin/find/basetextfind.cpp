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

#include "basetextfind.h"

#include <utils/qtcassert.h>
#include <utils/filesearch.h>

#include <QPointer>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>

namespace Core {

static QRegularExpression regularExpression(const QString &txt, FindFlags flags)
{
    return QRegularExpression(
                (flags & FindRegularExpression) ? txt
                                                : QRegularExpression::escape(txt),
                (flags & FindCaseSensitively) ? QRegularExpression::NoPatternOption
                                              : QRegularExpression::CaseInsensitiveOption);
}

struct BaseTextFindPrivate
{
    explicit BaseTextFindPrivate(QPlainTextEdit *editor);
    explicit BaseTextFindPrivate(QTextEdit *editor);

    QPointer<QTextEdit> m_editor;
    QPointer<QPlainTextEdit> m_plaineditor;
    QPointer<QWidget> m_widget;
    QTextCursor m_findScopeStart;
    QTextCursor m_findScopeEnd;
    int m_findScopeVerticalBlockSelectionFirstColumn;
    int m_findScopeVerticalBlockSelectionLastColumn;
    int m_incrementalStartPos;
    bool m_incrementalWrappedState;
};

BaseTextFindPrivate::BaseTextFindPrivate(QTextEdit *editor)
    : m_editor(editor)
    , m_widget(editor)
    , m_findScopeVerticalBlockSelectionFirstColumn(-1)
    , m_findScopeVerticalBlockSelectionLastColumn(-1)
    , m_incrementalStartPos(-1)
    , m_incrementalWrappedState(false)
{
}

BaseTextFindPrivate::BaseTextFindPrivate(QPlainTextEdit *editor)
    : m_plaineditor(editor)
    , m_widget(editor)
    , m_findScopeVerticalBlockSelectionFirstColumn(-1)
    , m_findScopeVerticalBlockSelectionLastColumn(-1)
    , m_incrementalStartPos(-1)
    , m_incrementalWrappedState(false)
{
}

BaseTextFind::BaseTextFind(QTextEdit *editor)
    : d(new BaseTextFindPrivate(editor))
{
}


BaseTextFind::BaseTextFind(QPlainTextEdit *editor)
    : d(new BaseTextFindPrivate(editor))
{
}

BaseTextFind::~BaseTextFind()
{
    delete d;
}

QTextCursor BaseTextFind::textCursor() const
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return QTextCursor());
    return d->m_editor ? d->m_editor->textCursor() : d->m_plaineditor->textCursor();
}

void BaseTextFind::setTextCursor(const QTextCursor &cursor)
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return);
    d->m_editor ? d->m_editor->setTextCursor(cursor) : d->m_plaineditor->setTextCursor(cursor);
}

QTextDocument *BaseTextFind::document() const
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return 0);
    return d->m_editor ? d->m_editor->document() : d->m_plaineditor->document();
}

bool BaseTextFind::isReadOnly() const
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return true);
    return d->m_editor ? d->m_editor->isReadOnly() : d->m_plaineditor->isReadOnly();
}

bool BaseTextFind::supportsReplace() const
{
    return !isReadOnly();
}

FindFlags BaseTextFind::supportedFindFlags() const
{
    return FindBackward | FindCaseSensitively | FindRegularExpression
            | FindWholeWords | FindPreserveCase;
}

void BaseTextFind::resetIncrementalSearch()
{
    d->m_incrementalStartPos = -1;
    d->m_incrementalWrappedState = false;
}

void BaseTextFind::clearHighlights()
{
    highlightAll(QString(), 0);
}

QString BaseTextFind::currentFindString() const
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.block() != cursor.document()->findBlock(cursor.anchor()))
        return QString(); // multi block selection

    if (cursor.hasSelection())
        return cursor.selectedText();

    if (!cursor.atBlockEnd() && !cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::StartOfWord);
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        QString s = cursor.selectedText();
        foreach (QChar c, s) {
            if (!c.isLetterOrNumber() && c != QLatin1Char('_')) {
                s.clear();
                break;
            }
        }
        return s;
    }

    return QString();
}

QString BaseTextFind::completedFindString() const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(textCursor().selectionStart());
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

IFindSupport::Result BaseTextFind::findIncremental(const QString &txt, FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    if (d->m_incrementalStartPos < 0)
        d->m_incrementalStartPos = cursor.selectionStart();
    cursor.setPosition(d->m_incrementalStartPos);
    bool wrapped = false;
    bool found =  find(txt, findFlags, cursor, &wrapped);
    if (wrapped != d->m_incrementalWrappedState && found) {
        d->m_incrementalWrappedState = wrapped;
        showWrapIndicator(d->m_widget);
    }
    if (found)
        highlightAll(txt, findFlags);
    else
        highlightAll(QString(), 0);
    return found ? Found : NotFound;
}

IFindSupport::Result BaseTextFind::findStep(const QString &txt, FindFlags findFlags)
{
    bool wrapped = false;
    bool found = find(txt, findFlags, textCursor(), &wrapped);
    if (wrapped)
        showWrapIndicator(d->m_widget);
    if (found) {
        d->m_incrementalStartPos = textCursor().selectionStart();
        d->m_incrementalWrappedState = false;
    }
    return found ? Found : NotFound;
}

void BaseTextFind::replace(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    setTextCursor(cursor);
}

// QTextCursor::insert moves all other QTextCursors that are the the insertion point forward.
// We do not want that for the replace operation, because then e.g. the find scope would move when
// replacing a match at the start.
static void insertTextAfterSelection(const QString &text, QTextCursor &cursor)
{
    // first insert after the cursor's selection end, then remove selection
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    QTextCursor insertCursor = cursor;
    insertCursor.beginEditBlock();
    insertCursor.setPosition(end);
    insertCursor.insertText(text);
    // change cursor to be behind the inserted text, like it would be when directly inserting
    cursor = insertCursor;
    // redo the selection, because that changed when inserting the text at the end...
    insertCursor.setPosition(start);
    insertCursor.setPosition(end, QTextCursor::KeepAnchor);
    insertCursor.removeSelectedText();
    insertCursor.endEditBlock();
}

QTextCursor BaseTextFind::replaceInternal(const QString &before, const QString &after,
                                          FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    bool usesRegExp = (findFlags & FindRegularExpression);
    bool preserveCase = (findFlags & FindPreserveCase);
    QRegularExpression regexp = regularExpression(before, findFlags);
    QRegularExpressionMatch match = regexp.match(cursor.selectedText());
    if (match.hasMatch()) {
        QString realAfter;
        if (usesRegExp)
            realAfter = Utils::expandRegExpReplacement(after, match.capturedTexts());
        else if (preserveCase)
            realAfter = Utils::matchCaseReplacement(cursor.selectedText(), after);
        else
            realAfter = after;
        int start = cursor.selectionStart();
        insertTextAfterSelection(realAfter, cursor);
        if ((findFlags & FindBackward) != 0)
            cursor.setPosition(start);
    }
    return cursor;
}

bool BaseTextFind::replaceStep(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    bool wrapped = false;
    bool found = find(before, findFlags, cursor, &wrapped);
    if (wrapped)
        showWrapIndicator(d->m_widget);
    return found;
}

int BaseTextFind::replaceAll(const QString &before, const QString &after, FindFlags findFlags)
{
    QTextCursor editCursor = textCursor();
    if (!d->m_findScopeStart.isNull())
        editCursor.setPosition(d->m_findScopeStart.position());
    else
        editCursor.movePosition(QTextCursor::Start);
    editCursor.beginEditBlock();
    int count = 0;
    bool usesRegExp = (findFlags & FindRegularExpression);
    bool preserveCase = (findFlags & FindPreserveCase);
    QRegularExpression regexp = regularExpression(before, findFlags);
    QTextCursor found = findOne(regexp, editCursor, textDocumentFlagsForFindFlags(findFlags));
    bool first = true;
    while (!found.isNull() && inScope(found.selectionStart(), found.selectionEnd())) {
        if (found == editCursor && !first) {
            if (editCursor.atEnd())
                break;
            // If the newly found QTextCursor is the same as recently edit one we have to move on,
            // otherwise we would run into an endless loop for some regular expressions
            // like ^ or \b.
            QTextCursor newPosCursor = editCursor;
            newPosCursor.movePosition(findFlags & FindBackward ?
                                          QTextCursor::PreviousCharacter :
                                          QTextCursor::NextCharacter);
            found = findOne(regexp, newPosCursor, textDocumentFlagsForFindFlags(findFlags));
            continue;
        }
        if (first)
            first = false;
        ++count;
        editCursor.setPosition(found.selectionStart());
        editCursor.setPosition(found.selectionEnd(), QTextCursor::KeepAnchor);
        QRegularExpressionMatch match = regexp.match(found.selectedText());

        QString realAfter;
        if (usesRegExp)
            realAfter = Utils::expandRegExpReplacement(after, match.capturedTexts());
        else if (preserveCase)
            realAfter = Utils::matchCaseReplacement(found.selectedText(), after);
        else
            realAfter = after;
        insertTextAfterSelection(realAfter, editCursor);
        found = findOne(regexp, editCursor, textDocumentFlagsForFindFlags(findFlags));
    }
    editCursor.endEditBlock();
    return count;
}

bool BaseTextFind::find(const QString &txt, FindFlags findFlags,
    QTextCursor start, bool *wrapped)
{
    if (txt.isEmpty()) {
        setTextCursor(start);
        return true;
    }
    QRegularExpression regexp = regularExpression(txt, findFlags);
    QTextCursor found = findOne(regexp, start, textDocumentFlagsForFindFlags(findFlags));
    if (wrapped)
        *wrapped = false;

    if (!d->m_findScopeStart.isNull()) {

        // scoped
        if (found.isNull() || !inScope(found.selectionStart(), found.selectionEnd())) {
            if ((findFlags & FindBackward) == 0)
                start.setPosition(d->m_findScopeStart.position());
            else
                start.setPosition(d->m_findScopeEnd.position());
            found = findOne(regexp, start, textDocumentFlagsForFindFlags(findFlags));
            if (found.isNull() || !inScope(found.selectionStart(), found.selectionEnd()))
                return false;
            if (wrapped)
                *wrapped = true;
        }
    } else {

        // entire document
        if (found.isNull()) {
            if ((findFlags & FindBackward) == 0)
                start.movePosition(QTextCursor::Start);
            else
                start.movePosition(QTextCursor::End);
            found = findOne(regexp, start, textDocumentFlagsForFindFlags(findFlags));
            if (found.isNull())
                return false;
            if (wrapped)
                *wrapped = true;
        }
    }
    if (!found.isNull())
        setTextCursor(found);
    return true;
}


// helper function. Works just like QTextDocument::find() but supports vertical block selection
QTextCursor BaseTextFind::findOne(const QRegularExpression &expr,
                                  const QTextCursor &from, QTextDocument::FindFlags options) const
{
    QTextCursor candidate = document()->find(expr, from, options);
    if (candidate.isNull())
        return candidate;

    if (d->m_findScopeVerticalBlockSelectionFirstColumn < 0)
        return candidate;
    forever {
        if (!inScope(candidate.selectionStart(), candidate.selectionEnd()))
            return candidate;
        bool inVerticalFindScope = false;
        QMetaObject::invokeMethod(d->m_plaineditor, "inFindScope", Qt::DirectConnection,
                                  Q_RETURN_ARG(bool, inVerticalFindScope),
                                  Q_ARG(QTextCursor, candidate));
        if (inVerticalFindScope)
            return candidate;

        QTextCursor newCandidate = document()->find(expr, candidate, options);
        if (newCandidate == candidate) {
            // When searching for regular expressions that match "zero length" strings (like ^ or \b)
            // we need to move away from the match before searching for the next one.
            candidate.movePosition(options & QTextDocument::FindBackward
                                   ? QTextCursor::PreviousCharacter
                                   : QTextCursor::NextCharacter);
            candidate = document()->find(expr, candidate, options);
        } else {
            candidate = newCandidate;
        }
    }
    return candidate;
}

bool BaseTextFind::inScope(int startPosition, int endPosition) const
{
    if (d->m_findScopeStart.isNull())
        return true;
    return (d->m_findScopeStart.position() <= startPosition
            && d->m_findScopeEnd.position() >= endPosition);
}

void BaseTextFind::defineFindScope()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.block() != cursor.document()->findBlock(cursor.anchor())) {
        d->m_findScopeStart = cursor;
        d->m_findScopeStart.setPosition(qMax(0, cursor.selectionStart()));
        d->m_findScopeEnd = cursor;
        d->m_findScopeEnd.setPosition(cursor.selectionEnd());
        d->m_findScopeVerticalBlockSelectionFirstColumn = -1;
        d->m_findScopeVerticalBlockSelectionLastColumn = -1;

        if (d->m_plaineditor && d->m_plaineditor->metaObject()->indexOfProperty("verticalBlockSelectionFirstColumn") >= 0) {
            d->m_findScopeVerticalBlockSelectionFirstColumn
                    = d->m_plaineditor->property("verticalBlockSelectionFirstColumn").toInt();
            d->m_findScopeVerticalBlockSelectionLastColumn
                    = d->m_plaineditor->property("verticalBlockSelectionLastColumn").toInt();
        }

        emit findScopeChanged(d->m_findScopeStart, d->m_findScopeEnd,
                              d->m_findScopeVerticalBlockSelectionFirstColumn,
                              d->m_findScopeVerticalBlockSelectionLastColumn);
        cursor.setPosition(d->m_findScopeStart.position());
        setTextCursor(cursor);
    } else {
        clearFindScope();
    }
}

void BaseTextFind::clearFindScope()
{
    d->m_findScopeStart = QTextCursor();
    d->m_findScopeEnd = QTextCursor();
    d->m_findScopeVerticalBlockSelectionFirstColumn = -1;
    d->m_findScopeVerticalBlockSelectionLastColumn = -1;
    emit findScopeChanged(d->m_findScopeStart, d->m_findScopeEnd,
                          d->m_findScopeVerticalBlockSelectionFirstColumn,
                          d->m_findScopeVerticalBlockSelectionLastColumn);
}

void BaseTextFind::highlightAll(const QString &txt, FindFlags findFlags)
{
    emit highlightAllRequested(txt, findFlags);
}

} // namespace Core
