/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "basetextfind.h"

#include <utils/qtcassert.h>
#include <utils/filesearch.h>

#include <QtCore/QPointer>

#include <QtGui/QTextBlock>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextCursor>

namespace Find {

struct BaseTextFindPrivate {
    explicit BaseTextFindPrivate(QPlainTextEdit *editor);
    explicit BaseTextFindPrivate(QTextEdit *editor);

    QPointer<QTextEdit> m_editor;
    QPointer<QPlainTextEdit> m_plaineditor;
    QTextCursor m_findScopeStart;
    QTextCursor m_findScopeEnd;
    int m_findScopeVerticalBlockSelectionFirstColumn;
    int m_findScopeVerticalBlockSelectionLastColumn;
    int m_incrementalStartPos;
};

BaseTextFindPrivate::BaseTextFindPrivate(QTextEdit *editor)
    : m_editor(editor)
    , m_findScopeVerticalBlockSelectionFirstColumn(-1)
    , m_findScopeVerticalBlockSelectionLastColumn(-1)
    , m_incrementalStartPos(-1)
{
}

BaseTextFindPrivate::BaseTextFindPrivate(QPlainTextEdit *editor)
    : m_plaineditor(editor)
    , m_findScopeVerticalBlockSelectionFirstColumn(-1)
    , m_findScopeVerticalBlockSelectionLastColumn(-1)
    , m_incrementalStartPos(-1)
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
}

QTextCursor BaseTextFind::textCursor() const
{
    QTC_ASSERT(d->m_editor || d->m_plaineditor, return QTextCursor());
    return d->m_editor ? d->m_editor->textCursor() : d->m_plaineditor->textCursor();

}

void BaseTextFind::setTextCursor(const QTextCursor& cursor)
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
            | Find::FindRegularExpression | Find::FindWholeWords;
}

void BaseTextFind::resetIncrementalSearch()
{
    d->m_incrementalStartPos = -1;
}

void BaseTextFind::clearResults()
{
    emit highlightAll(QString(), 0);
}

QString BaseTextFind::currentFindString() const
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.block() != cursor.document()->findBlock(cursor.anchor())) {
        return QString(); // multi block selection
    }

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
    bool found =  find(txt, findFlags, cursor);
    if (found)
        emit highlightAll(txt, findFlags);
    else
        emit highlightAll(QString(), 0);
    return found ? Found : NotFound;
}

IFindSupport::Result BaseTextFind::findStep(const QString &txt, Find::FindFlags findFlags)
{
    bool found = find(txt, findFlags, textCursor());
    if (found)
        d->m_incrementalStartPos = textCursor().selectionStart();
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
    QRegExp regexp(before,
                   (findFlags & Find::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive,
                   usesRegExp ? QRegExp::RegExp : QRegExp::FixedString);

    if (regexp.exactMatch(cursor.selectedText())) {
        QString realAfter = usesRegExp ? Utils::expandRegExpReplacement(after, regexp.capturedTexts()) : after;
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
    return find(before, findFlags, cursor);
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
    QRegExp regexp(before);
    regexp.setPatternSyntax(usesRegExp ? QRegExp::RegExp : QRegExp::FixedString);
    regexp.setCaseSensitivity((findFlags & Find::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive);
    QTextCursor found = findOne(regexp, editCursor, Find::textDocumentFlagsForFindFlags(findFlags));
    while (!found.isNull() && found.selectionStart() < found.selectionEnd()
            && inScope(found.selectionStart(), found.selectionEnd())) {
        ++count;
        editCursor.setPosition(found.selectionStart());
        editCursor.setPosition(found.selectionEnd(), QTextCursor::KeepAnchor);
        regexp.exactMatch(found.selectedText());
        QString realAfter = usesRegExp ? Utils::expandRegExpReplacement(after, regexp.capturedTexts()) : after;
        editCursor.insertText(realAfter);
        found = findOne(regexp, editCursor, Find::textDocumentFlagsForFindFlags(findFlags));
    }
    editCursor.endEditBlock();
    return count;
}

bool BaseTextFind::find(const QString &txt,
                               Find::FindFlags findFlags,
                               QTextCursor start)
{
    if (txt.isEmpty()) {
        setTextCursor(start);
        return true;
    }
    QRegExp regexp(txt);
    regexp.setPatternSyntax((findFlags&Find::FindRegularExpression) ? QRegExp::RegExp : QRegExp::FixedString);
    regexp.setCaseSensitivity((findFlags&Find::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive);
    QTextCursor found = findOne(regexp, start, Find::textDocumentFlagsForFindFlags(findFlags));

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
        }
    } else {

        // entire document
        if (found.isNull()) {
            if ((findFlags&Find::FindBackward) == 0)
                start.movePosition(QTextCursor::Start);
            else
                start.movePosition(QTextCursor::End);
            found = findOne(regexp, start, Find::textDocumentFlagsForFindFlags(findFlags));
            if (found.isNull()) {
                return false;
            }
        }
    }
    if (!found.isNull()) {
        setTextCursor(found);
    }
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
        candidate = document()->find(expr, candidate, options);
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
