/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "basetextfind.h"

#include <utils/qtcassert.h>

#include <QtGui/QTextBlock>

using namespace Find;

BaseTextFind::BaseTextFind(QTextEdit *editor)
    : m_editor(editor), m_incrementalStartPos(-1)
{
}

BaseTextFind::BaseTextFind(QPlainTextEdit *editor)
    : m_plaineditor(editor), m_incrementalStartPos(-1)
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

bool BaseTextFind::findIncremental(const QString &txt, QTextDocument::FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    if (m_incrementalStartPos < 0)
        m_incrementalStartPos = cursor.selectionStart();
    cursor.setPosition(m_incrementalStartPos);
    findFlags &= ~QTextDocument::FindBackward;
    bool found =  find(txt, findFlags, cursor);
    if (found)
        emit highlightAll(txt, findFlags);
    else
        emit highlightAll(QString(), 0);
    return found;
}

bool BaseTextFind::findStep(const QString &txt, QTextDocument::FindFlags findFlags)
{
    bool found = find(txt, findFlags, textCursor());
    if (found)
        m_incrementalStartPos = textCursor().selectionStart();
    return found;
}

bool BaseTextFind::replaceStep(const QString &before, const QString &after,
    QTextDocument::FindFlags findFlags)
{
    QTextCursor cursor = textCursor();
    if (cursor.selectedText().compare(before,
            ((findFlags&QTextDocument::FindCaseSensitively)!=0) ? Qt::CaseSensitive : Qt::CaseInsensitive) == 0) {
        int start = cursor.selectionStart();
        cursor.insertText(after);
        if ((findFlags&QTextDocument::FindBackward) != 0)
            cursor.setPosition(start);
    }
    return find(before, findFlags, cursor);
}

int BaseTextFind::replaceAll(const QString &before, const QString &after,
    QTextDocument::FindFlags findFlags)
{
    QTextCursor editCursor = textCursor();
    if (!m_findScope.isNull())
        editCursor.setPosition(m_findScope.selectionStart());
    else
        editCursor.movePosition(QTextCursor::Start);
    editCursor.beginEditBlock();
    int count = 0;
    QTextCursor found;
    found = document()->find(before, editCursor, findFlags);
    while (!found.isNull() && inScope(found.selectionStart(), found.selectionEnd())) {
        ++count;
        editCursor.setPosition(found.selectionStart());
        editCursor.setPosition(found.selectionEnd(), QTextCursor::KeepAnchor);
        editCursor.insertText(after);
        found = document()->find(before, editCursor, findFlags);
    }
    editCursor.endEditBlock();
    return count;
}

bool BaseTextFind::find(const QString &txt,
                               QTextDocument::FindFlags findFlags,
                               QTextCursor start)
{
    if (txt.isEmpty()) {
        setTextCursor(start);
        return true;
    }
    QTextCursor found = document()->find(txt, start, findFlags);

    if (!m_findScope.isNull()) {

        // scoped
        if (found.isNull() || !inScope(found.selectionStart(), found.selectionEnd())) {
            if ((findFlags&QTextDocument::FindBackward) == 0)
                start.setPosition(m_findScope.selectionStart());
            else
                start.setPosition(m_findScope.selectionEnd());
            found = document()->find(txt, start, findFlags);
            if (found.isNull() || !inScope(found.selectionStart(), found.selectionEnd()))
                return false;
        }
    } else {

        // entire document
        if (found.isNull()) {
            if ((findFlags&QTextDocument::FindBackward) == 0)
                start.movePosition(QTextCursor::Start);
            else
                start.movePosition(QTextCursor::End);
            found = document()->find(txt, start, findFlags);
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

bool BaseTextFind::inScope(int startPosition, int endPosition) const
{
    if (m_findScope.isNull())
        return true;
    return (m_findScope.selectionStart() <= startPosition
            && m_findScope.selectionEnd() >= endPosition);
}

void BaseTextFind::defineFindScope()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.block() != cursor.document()->findBlock(cursor.anchor())) {
        m_findScope = cursor;
        emit findScopeChanged(m_findScope);
        cursor.setPosition(cursor.selectionStart());
        setTextCursor(cursor);
    } else {
        clearFindScope();
    }
}

void BaseTextFind::clearFindScope()
{
    m_findScope = QTextCursor();
    emit findScopeChanged(m_findScope);
}
