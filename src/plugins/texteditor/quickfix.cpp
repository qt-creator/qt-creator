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

#include "quickfix.h"
#include "basetexteditor.h"

#include <QtGui/QApplication>
#include <QtGui/QTextBlock>

#include <QtCore/QDebug>

using TextEditor::QuickFixOperation;

QuickFixOperation::QuickFixOperation(TextEditor::BaseTextEditor *editor)
    : _editor(editor)
{
}

QuickFixOperation::~QuickFixOperation()
{
}

TextEditor::BaseTextEditor *QuickFixOperation::editor() const
{
    return _editor;
}

QTextCursor QuickFixOperation::textCursor() const
{
    return _textCursor;
}

void QuickFixOperation::setTextCursor(const QTextCursor &cursor)
{
    _textCursor = cursor;
}

int QuickFixOperation::selectionStart() const
{
    return _textCursor.selectionStart();
}

int QuickFixOperation::selectionEnd() const
{
    return _textCursor.selectionEnd();
}

const Utils::ChangeSet &QuickFixOperation::changeSet() const
{
    return _changeSet;
}

void QuickFixOperation::apply()
{
    const Range r = topLevelRange();

    _textCursor.beginEditBlock();
    _changeSet.apply(&_textCursor);
    reindent(r);
    _textCursor.endEditBlock();
}

QuickFixOperation::Range QuickFixOperation::range(int from, int to) const
{
    QTextDocument *doc = editor()->document();

    QTextCursor begin(doc);
    begin.setPosition(from);

    QTextCursor end(doc);
    end.setPosition(to);

    Range range;
    range.begin = begin;
    range.end = end;
    return range;
}

int QuickFixOperation::position(int line, int column) const
{
    QTextDocument *doc = editor()->document();
    return doc->findBlockByNumber(line - 1).position() + column - 1;
}

void QuickFixOperation::reindent(const Range &range)
{
    if (! range.isNull()) {
        QTextCursor tc = range.begin;
        tc.setPosition(range.end.position(), QTextCursor::KeepAnchor);
        editor()->indentInsertedText(tc);
    }
}

void QuickFixOperation::move(int start, int end, int to)
{
    _changeSet.move(start, end-start, to);
}

void QuickFixOperation::replace(int start, int end, const QString &replacement)
{
    _changeSet.replace(start, end-start, replacement);
}

void QuickFixOperation::insert(int at, const QString &text)
{
    _changeSet.insert(at, text);
}

void QuickFixOperation::remove(int start, int end)
{
    _changeSet.remove(start, end-start);
}

void QuickFixOperation::flip(int start1, int end1, int start2, int end2)
{
    _changeSet.flip(start1, end1-start1, start2, end2-start2);
}

void QuickFixOperation::copy(int start, int end, int to)
{
    _changeSet.copy(start, end-start, to);
}

QChar QuickFixOperation::charAt(int offset) const
{
    QTextDocument *doc = _textCursor.document();
    return doc->characterAt(offset);
}

QString QuickFixOperation::textOf(int start, int end) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(start);
    tc.setPosition(end, QTextCursor::KeepAnchor);
    return tc.selectedText();
}

void QuickFixOperation::perform()
{
    createChangeSet();
    apply();
}
