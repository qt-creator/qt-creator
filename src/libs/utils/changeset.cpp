/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "changeset.h"

namespace Utils {

ChangeSet::ChangeSet()
    : m_string(0), m_cursor(0), m_error(false)
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

bool ChangeSet::replace(int pos, int length, const QString &replacement)
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

bool ChangeSet::move(int pos, int length, int to)
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
    if (hasOverlap(pos, 0))
        m_error = true;

    EditOp cmd(EditOp::Insert);
    cmd.pos1 = pos;
    cmd.text = text;
    m_operationList += cmd;

    return !m_error;
}

bool ChangeSet::remove(int pos, int length)
{
    if (hasOverlap(pos, length))
        m_error = true;

    EditOp cmd(EditOp::Remove);
    cmd.pos1 = pos;
    cmd.length1 = length;
    m_operationList += cmd;

    return !m_error;
}

bool ChangeSet::flip(int pos1, int length1, int pos2, int length2)
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

bool ChangeSet::copy(int pos, int length, int to)
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

void ChangeSet::doReplace(const EditOp &replace, QList<EditOp> *replaceList)
{
    Q_ASSERT(replace.type == EditOp::Replace);

    {
        QMutableListIterator<EditOp> i(*replaceList);
        while (i.hasNext()) {
            EditOp &c = i.next();
            if (replace.pos1 <= c.pos1)
                c.pos1 += replace.text.size();
            if (replace.pos1 < c.pos1)
                c.pos1 -= replace.length1;
        }
    }

    if (m_string) {
        m_string->replace(replace.pos1, replace.length1, replace.text);
    } else if (m_cursor) {
        m_cursor->setPosition(replace.pos1);
        m_cursor->setPosition(replace.pos1 + replace.length1, QTextCursor::KeepAnchor);
        m_cursor->insertText(replace.text);
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
