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

#include "changeset.h"

#include <QTextCursor>

namespace Utils {

ChangeSet::ChangeSet()
    : m_string(0), m_cursor(0), m_error(false)
{
}

ChangeSet::ChangeSet(const QList<EditOp> &operations)
    : m_string(0), m_cursor(0), m_operationList(operations), m_error(false)
{
}

static bool overlaps(int posA, int lengthA, int posB, int lengthB) {
    if (lengthB > 0) {
        return
                // right edge of B contained in A
                (posA < posB + lengthB && posA + lengthA >= posB + lengthB)
                // left edge of B contained in A
                || (posA <= posB && posA + lengthA > posB)
                // A contained in B
                || (posB < posA && posB + lengthB > posA + lengthA);
    } else {
        return (posB > posA && posB < posA + lengthA);
    }
}

bool ChangeSet::hasOverlap(int pos, int length)
{
    QListIterator<EditOp> i(m_operationList);
    while (i.hasNext()) {
        const EditOp &cmd = i.next();

        switch (cmd.type) {
        case EditOp::Replace:
            if (overlaps(pos, length, cmd.pos1, cmd.length1))
                return true;
            break;

        case EditOp::Move:
            if (overlaps(pos, length, cmd.pos1, cmd.length1))
                return true;
            if (cmd.pos2 > pos && cmd.pos2 < pos + length)
                return true;
            break;

        case EditOp::Insert:
            if (cmd.pos1 > pos && cmd.pos1 < pos + length)
                return true;
            break;

        case EditOp::Remove:
            if (overlaps(pos, length, cmd.pos1, cmd.length1))
                return true;
            break;

        case EditOp::Flip:
            if (overlaps(pos, length, cmd.pos1, cmd.length1))
                return true;
            if (overlaps(pos, length, cmd.pos2, cmd.length2))
                return true;
            break;

        case EditOp::Copy:
            if (overlaps(pos, length, cmd.pos1, cmd.length1))
                return true;
            if (cmd.pos2 > pos && cmd.pos2 < pos + length)
                return true;
            break;

        case EditOp::Unset:
            break;
        }
    }

    return false;
}

bool ChangeSet::isEmpty() const
{
    return m_operationList.isEmpty();
}

QList<ChangeSet::EditOp> ChangeSet::operationList() const
{
    return m_operationList;
}

void ChangeSet::clear()
{
    m_string = 0;
    m_cursor = 0;
    m_operationList.clear();
    m_error = false;
}

bool ChangeSet::replace_helper(int pos, int length, const QString &replacement)
{
    if (hasOverlap(pos, length))
        m_error = true;

    EditOp cmd(EditOp::Replace);
    cmd.pos1 = pos;
    cmd.length1 = length;
    cmd.text = replacement;
    m_operationList += cmd;

    return !m_error;
}

bool ChangeSet::move_helper(int pos, int length, int to)
{
    if (hasOverlap(pos, length)
        || hasOverlap(to, 0)
        || overlaps(pos, length, to, 0))
        m_error = true;

    EditOp cmd(EditOp::Move);
    cmd.pos1 = pos;
    cmd.length1 = length;
    cmd.pos2 = to;
    m_operationList += cmd;

    return !m_error;
}

bool ChangeSet::insert(int pos, const QString &text)
{
    Q_ASSERT(pos >= 0);

    if (hasOverlap(pos, 0))
        m_error = true;

    EditOp cmd(EditOp::Insert);
    cmd.pos1 = pos;
    cmd.text = text;
    m_operationList += cmd;

    return !m_error;
}

bool ChangeSet::replace(const Range &range, const QString &replacement)
{ return replace(range.start, range.end, replacement); }

bool ChangeSet::remove(const Range &range)
{ return remove(range.start, range.end); }

bool ChangeSet::move(const Range &range, int to)
{ return move(range.start, range.end, to); }

bool ChangeSet::flip(const Range &range1, const Range &range2)
{ return flip(range1.start, range1.end, range2.start, range2.end); }

bool ChangeSet::copy(const Range &range, int to)
{ return copy(range.start, range.end, to); }

bool ChangeSet::replace(int start, int end, const QString &replacement)
{ return replace_helper(start, end - start, replacement); }

bool ChangeSet::remove(int start, int end)
{ return remove_helper(start, end - start); }

bool ChangeSet::move(int start, int end, int to)
{ return move_helper(start, end - start, to); }

bool ChangeSet::flip(int start1, int end1, int start2, int end2)
{ return flip_helper(start1, end1 - start1, start2, end2 - start2); }

bool ChangeSet::copy(int start, int end, int to)
{ return copy_helper(start, end - start, to); }

bool ChangeSet::remove_helper(int pos, int length)
{
    if (hasOverlap(pos, length))
        m_error = true;

    EditOp cmd(EditOp::Remove);
    cmd.pos1 = pos;
    cmd.length1 = length;
    m_operationList += cmd;

    return !m_error;
}

bool ChangeSet::flip_helper(int pos1, int length1, int pos2, int length2)
{
    if (hasOverlap(pos1, length1)
        || hasOverlap(pos2, length2)
        || overlaps(pos1, length1, pos2, length2))
        m_error = true;

    EditOp cmd(EditOp::Flip);
    cmd.pos1 = pos1;
    cmd.length1 = length1;
    cmd.pos2 = pos2;
    cmd.length2 = length2;
    m_operationList += cmd;

    return !m_error;
}

bool ChangeSet::copy_helper(int pos, int length, int to)
{
    if (hasOverlap(pos, length)
        || hasOverlap(to, 0)
        || overlaps(pos, length, to, 0))
        m_error = true;

    EditOp cmd(EditOp::Copy);
    cmd.pos1 = pos;
    cmd.length1 = length;
    cmd.pos2 = to;
    m_operationList += cmd;

    return !m_error;
}

void ChangeSet::doReplace(const EditOp &replace_helper, QList<EditOp> *replaceList)
{
    Q_ASSERT(replace_helper.type == EditOp::Replace);

    {
        QMutableListIterator<EditOp> i(*replaceList);
        while (i.hasNext()) {
            EditOp &c = i.next();
            if (replace_helper.pos1 <= c.pos1)
                c.pos1 += replace_helper.text.size();
            if (replace_helper.pos1 < c.pos1)
                c.pos1 -= replace_helper.length1;
        }
    }

    if (m_string) {
        m_string->replace(replace_helper.pos1, replace_helper.length1, replace_helper.text);
    } else if (m_cursor) {
        m_cursor->setPosition(replace_helper.pos1);
        m_cursor->setPosition(replace_helper.pos1 + replace_helper.length1, QTextCursor::KeepAnchor);
        m_cursor->insertText(replace_helper.text);
    }
}

void ChangeSet::convertToReplace(const EditOp &op, QList<EditOp> *replaceList)
{
    EditOp replace1(EditOp::Replace);
    EditOp replace2(EditOp::Replace);

    switch (op.type) {
    case EditOp::Replace:
        replaceList->append(op);
        break;

    case EditOp::Move:
        replace1.pos1 = op.pos1;
        replace1.length1 = op.length1;
        replaceList->append(replace1);

        replace2.pos1 = op.pos2;
        replace2.text = textAt(op.pos1, op.length1);
        replaceList->append(replace2);
        break;

    case EditOp::Insert:
        replace1.pos1 = op.pos1;
        replace1.text = op.text;
        replaceList->append(replace1);
        break;

    case EditOp::Remove:
        replace1.pos1 = op.pos1;
        replace1.length1 = op.length1;
        replaceList->append(replace1);
        break;

    case EditOp::Flip:
        replace1.pos1 = op.pos1;
        replace1.length1 = op.length1;
        replace1.text = textAt(op.pos2, op.length2);
        replaceList->append(replace1);

        replace2.pos1 = op.pos2;
        replace2.length1 = op.length2;
        replace2.text = textAt(op.pos1, op.length1);
        replaceList->append(replace2);
        break;

    case EditOp::Copy:
        replace1.pos1 = op.pos2;
        replace1.text = textAt(op.pos1, op.length1);
        replaceList->append(replace1);
        break;

    case EditOp::Unset:
        break;
    }
}

bool ChangeSet::hadErrors()
{
    return m_error;
}

void ChangeSet::apply(QString *s)
{
    m_string = s;
    apply_helper();
    m_string = 0;
}

void ChangeSet::apply(QTextCursor *textCursor)
{
    m_cursor = textCursor;
    apply_helper();
    m_cursor = 0;
}

QString ChangeSet::textAt(int pos, int length)
{
    if (m_string) {
        return m_string->mid(pos, length);
    } else if (m_cursor) {
        m_cursor->setPosition(pos);
        m_cursor->setPosition(pos + length, QTextCursor::KeepAnchor);
        return m_cursor->selectedText();
    }
    return QString();
}

void ChangeSet::apply_helper()
{
    // convert all ops to replace
    QList<EditOp> replaceList;
    {
        while (!m_operationList.isEmpty()) {
            const EditOp cmd(m_operationList.first());
            m_operationList.removeFirst();
            convertToReplace(cmd, &replaceList);
        }
    }

    // execute replaces
    if (m_cursor)
        m_cursor->beginEditBlock();

    while (!replaceList.isEmpty()) {
        const EditOp cmd(replaceList.first());
        replaceList.removeFirst();
        doReplace(cmd, &replaceList);
    }

    if (m_cursor)
        m_cursor->endEditBlock();
}

} // end namespace Utils
