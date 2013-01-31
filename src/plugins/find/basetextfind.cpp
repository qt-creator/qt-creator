/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basetextfind.h"

#include <utils/qtcassert.h>
#include <utils/filesearch.h>

#include <QPointer>

#include <QTextBlock>
#include <QPlainTextEdit>
#include <QTextCursor>

namespace Find {

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

Find::FindFlags BaseTextFind::supportedFindFlags() const
{
    return Find::FindBackward | Find::FindCaseSensitively
            | Find::FindRegularExpression | Find::FindWholeWords
            | Find::FindPreserveCase;
}

void BaseTextFind::resetIncrementalSearch()
{
    d->m_incrementalStartPos = -1;
    d->m_incrementalWrappedState = false;
}

void BaseTextFind::clearResults()
{
    emit highlightAll(QString(), 0);
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

IFindSupport::Result BaseTextFind::findIncremental(const QString &txt, Find::FindFlags findFlags)
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
        emit highlightAll(txt, findFlags);
    else
        emit highlightAll(QString(), 0);
    return found ? Found : NotFound;
}

IFindSupport::Result BaseTextFind::findStep(const QString &txt, Find::FindFlags findFlags)
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

void BaseTextFind::replace(const QString &before, const QString &after,
                           Find::FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    setTextCursor(cursor);
}

QTextCursor BaseTextFind::replaceInternal(const QString &before, const QString &after,
                                          Find::FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    bool usesRegExp = (findFlags & Find::FindRegularExpression);
    bool preserveCase = (findFlags & Find::FindPreserveCase);
    QRegExp regexp(before,
                   (findFlags & Find::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive,
                   usesRegExp ? QRegExp::RegExp : QRegExp::FixedString);

    if (regexp.exactMatch(cursor.selectedText())) {
        QString realAfter;
        if (usesRegExp)
            realAfter = Utils::expandRegExpReplacement(after, regexp.capturedTexts());
        else if (preserveCase)
            realAfter = Utils::matchCaseReplacement(cursor.selectedText(), after);
        else
            realAfter = after;
        int start = cursor.selectionStart();
        cursor.insertText(realAfter);
        if ((findFlags&Find::FindBackward) != 0)
            cursor.setPosition(start);
    }
    return cursor;
}

bool BaseTextFind::replaceStep(const QString &before, const QString &after,
    Find::FindFlags findFlags)
{
    QTextCursor cursor = replaceInternal(before, after, findFlags);
    bool wrapped = false;
    bool found = find(before, findFlags, cursor, &wrapped);
    if (wrapped)
        showWrapIndicator(d->m_widget);
    return found;
}

int BaseTextFind::replaceAll(const QString &before, const QString &after,
    Find::FindFlags findFlags)
{
    QTextCursor editCursor = textCursor();
    if (!d->m_findScopeStart.isNull())
        editCursor.setPosition(d->m_findScopeStart.position());
    else
        editCursor.movePosition(QTextCursor::Start);
    editCursor.beginEditBlock();
    int count = 0;
    bool usesRegExp = (findFlags & Find::FindRegularExpression);
    bool preserveCase = (findFlags & Find::FindPreserveCase);
    QRegExp regexp(before);
    regexp.setPatternSyntax(usesRegExp ? QRegExp::RegExp : QRegExp::FixedString);
    regexp.setCaseSensitivity((findFlags & Find::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive);
    QTextCursor found = findOne(regexp, editCursor, Find::textDocumentFlagsForFindFlags(findFlags));
    bool first = true;
    while (!found.isNull() && inScope(found.selectionStart(), found.selectionEnd())) {
        if (found == editCursor && !first) {
            if (editCursor.atEnd())
                break;
            // If the newly found QTextCursor is the same as recently edit one we have to move on,
            // otherwise we would run into an endless loop for some regular expressions
            // like ^ or \b.
            QTextCursor newPosCursor = editCursor;
            newPosCursor.movePosition(findFlags & Find::FindBackward ?
                                          QTextCursor::PreviousCharacter :
                                          QTextCursor::NextCharacter);
            found = findOne(regexp, newPosCursor, Find::textDocumentFlagsForFindFlags(findFlags));
            continue;
        }
        if (first)
            first = false;
        ++count;
        editCursor.setPosition(found.selectionStart());
        editCursor.setPosition(found.selectionEnd(), QTextCursor::KeepAnchor);
        regexp.exactMatch(found.selectedText());

        QString realAfter;
        if (usesRegExp)
            realAfter = Utils::expandRegExpReplacement(after, regexp.capturedTexts());
        else if (preserveCase)
            realAfter = Utils::matchCaseReplacement(found.selectedText(), after);
        else
            realAfter = after;
        editCursor.insertText(realAfter);
        found = findOne(regexp, editCursor, Find::textDocumentFlagsForFindFlags(findFlags));
    }
    editCursor.endEditBlock();
    return count;
}

bool BaseTextFind::find(const QString &txt, Find::FindFlags findFlags,
    QTextCursor start, bool *wrapped)
{
    if (txt.isEmpty()) {
        setTextCursor(start);
        return true;
    }
    QRegExp regexp(txt);
    regexp.setPatternSyntax((findFlags&Find::FindRegularExpression) ? QRegExp::RegExp : QRegExp::FixedString);
    regexp.setCaseSensitivity((findFlags&Find::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive);
    QTextCursor found = findOne(regexp, start, Find::textDocumentFlagsForFindFlags(findFlags));
    if (wrapped)
        *wrapped = false;

    if (!d->m_findScopeStart.isNull()) {

        // scoped
        if (found.isNull() || !inScope(found.selectionStart(), found.selectionEnd())) {
            if ((findFlags&Find::FindBackward) == 0)
                start.setPosition(d->m_findScopeStart.position());
            else
                start.setPosition(d->m_findScopeEnd.position());
            found = findOne(regexp, start, Find::textDocumentFlagsForFindFlags(findFlags));
            if (found.isNull() || !inScope(found.selectionStart(), found.selectionEnd()))
                return false;
            if (wrapped)
                *wrapped = true;
        }
    } else {

        // entire document
        if (found.isNull()) {
            if ((findFlags&Find::FindBackward) == 0)
                start.movePosition(QTextCursor::Start);
            else
                start.movePosition(QTextCursor::End);
            found = findOne(regexp, start, Find::textDocumentFlagsForFindFlags(findFlags));
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
QTextCursor BaseTextFind::findOne(const QRegExp &expr, const QTextCursor &from, QTextDocument::FindFlags options) const
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
        d->m_findScopeStart = QTextCursor(document()->docHandle(), qMax(0, cursor.selectionStart()));
        d->m_findScopeEnd = QTextCursor(document()->docHandle(), cursor.selectionEnd());
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

} // namespace Find
