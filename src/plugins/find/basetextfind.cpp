/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "basetextfind.h"

#include <utils/qtcassert.h>
#include <utils/filesearch.h>

#include <QtGui/QTextBlock>
#include <QtGui/QPlainTextEdit>

using namespace Find;

BaseTextFind::BaseTextFind(QTextEdit *editor)
    : m_editor(editor)
    , m_findScopeVerticalBlockSelection(0)
    , m_incrementalStartPos(-1)
{
}

BaseTextFind::BaseTextFind(QPlainTextEdit *editor)
    : m_plaineditor(editor)
    , m_findScopeVerticalBlockSelection(0)
    , m_incrementalStartPos(-1)
{
}

QTextCursor BaseTextFind::textCursor() const
{
    QTC_ASSERT(m_editor || m_plaineditor, return QTextCursor());
    return m_editor ? m_editor->textCursor() : m_plaineditor->textCursor();

}

void BaseTextFind::setTextCursor(const QTextCursor& cursor)
{
    QTC_ASSERT(m_editor || m_plaineditor, return);
    m_editor ? m_editor->setTextCursor(cursor) : m_plaineditor->setTextCursor(cursor);
}

QTextDocument *BaseTextFind::document() const
{
    QTC_ASSERT(m_editor || m_plaineditor, return 0);
    return m_editor ? m_editor->document() : m_plaineditor->document();
}

bool BaseTextFind::isReadOnly() const
{
    QTC_ASSERT(m_editor || m_plaineditor, return true);
    return m_editor ? m_editor->isReadOnly() : m_plaineditor->isReadOnly();
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
    m_incrementalStartPos = -1;
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
    if (m_incrementalStartPos < 0)
        m_incrementalStartPos = cursor.selectionStart();
    cursor.setPosition(m_incrementalStartPos);
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
        m_incrementalStartPos = textCursor().selectionStart();
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
    if (!m_findScopeStart.isNull())
        editCursor.setPosition(m_findScopeStart.position());
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

    if (!m_findScopeStart.isNull()) {

        // scoped
        if (found.isNull() || !inScope(found.selectionStart(), found.selectionEnd())) {
            if ((findFlags&Find::FindBackward) == 0)
                start.setPosition(m_findScopeStart.position());
            else
                start.setPosition(m_findScopeEnd.position());
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

    if (!m_findScopeVerticalBlockSelection)
        return candidate;
    forever {
        if (!inScope(candidate.selectionStart(), candidate.selectionEnd()))
            return candidate;
        QTextCursor b = candidate;
        b.setPosition(candidate.selectionStart());
        QTextCursor e = candidate;
        e.setPosition(candidate.selectionEnd());
        if (b.positionInBlock() >= m_findScopeStart.positionInBlock() + 1
            && e.positionInBlock() <= m_findScopeStart.positionInBlock() + 1 + m_findScopeVerticalBlockSelection)
            return candidate;
        candidate = document()->find(expr, candidate, options);
    }
    return candidate;
}

bool BaseTextFind::inScope(int startPosition, int endPosition) const
{
    if (m_findScopeStart.isNull())
        return true;
    return (m_findScopeStart.position() <= startPosition
            && m_findScopeEnd.position() >= endPosition);
}

void BaseTextFind::defineFindScope()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.block() != cursor.document()->findBlock(cursor.anchor())) {
        m_findScopeStart = QTextCursor(document()->docHandle(), qMax(0, cursor.selectionStart()-1));
        m_findScopeEnd = QTextCursor(document()->docHandle(), cursor.selectionEnd());
        m_findScopeVerticalBlockSelection = 0;

        int verticalBlockSelection = 0;
        if (m_plaineditor && m_plaineditor->metaObject()->indexOfProperty("verticalBlockSelection") >= 0)
            verticalBlockSelection = m_plaineditor->property("verticalBlockSelection").toInt();

        if (verticalBlockSelection) {
            QTextCursor findScopeVisualStart(document()->docHandle(), cursor.selectionStart());
            int findScopeFromColumn = qMin(findScopeVisualStart.positionInBlock(),
                                         m_findScopeEnd.positionInBlock());
            int findScopeToColumn = findScopeFromColumn + verticalBlockSelection;
            m_findScopeStart.setPosition(findScopeVisualStart.block().position() + findScopeFromColumn - 1);
            m_findScopeEnd.setPosition(m_findScopeEnd.block().position()
                                       + qMin(m_findScopeEnd.block().length()-1, findScopeToColumn));
            m_findScopeVerticalBlockSelection = verticalBlockSelection;
        }
        emit findScopeChanged(m_findScopeStart, m_findScopeEnd, m_findScopeVerticalBlockSelection);
        cursor.setPosition(m_findScopeStart.position()+1);
        setTextCursor(cursor);
    } else {
        clearFindScope();
    }
}

void BaseTextFind::clearFindScope()
{
    m_findScopeStart = QTextCursor();
    m_findScopeEnd = QTextCursor();
    m_findScopeVerticalBlockSelection = 0;
    emit findScopeChanged(m_findScopeStart, m_findScopeEnd, m_findScopeVerticalBlockSelection);
}
