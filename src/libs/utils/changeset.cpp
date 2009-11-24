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
        :string(0), cursor(0)
{
}

static bool overlaps(int posA, int lengthA, int posB, int lengthB) {
    return (posA < posB + lengthB && posA + lengthA > posB + lengthB)
            || (posA < posB && posA + lengthA > posB);
}

bool ChangeSet::hasOverlap(int pos, int length)
{
    {
        QListIterator<Replace> i(m_replaceList);
        while (i.hasNext()) {
            const Replace &cmd = i.next();
            if (overlaps(pos, length, cmd.pos, cmd.length))
                return true;
        }
    }
    {
        QListIterator<Move> i(m_moveList);
        while (i.hasNext()) {
            const Move &cmd = i.next();
            if (overlaps(pos, length, cmd.pos, cmd.length))
                return true;
        }
        return false;
    }
}

bool ChangeSet::hasMoveInto(int pos, int length)
{
    QListIterator<Move> i(m_moveList);
    while (i.hasNext()) {
        const Move &cmd = i.next();
        if (cmd.to >= pos && cmd.to < pos + length)
            return true;
    }
    return false;
}

QList<ChangeSet::Replace> ChangeSet::replaceList() const
{
    return m_replaceList;
}

QList<ChangeSet::Move> ChangeSet::moveList() const
{
    return m_moveList;
}

void ChangeSet::replace(int pos, int length, const QString &replacement)
{
    Q_ASSERT(!hasOverlap(pos, length));
    Q_ASSERT(!hasMoveInto(pos, length));

    Replace cmd;
    cmd.pos = pos;
    cmd.length = length;
    cmd.replacement = replacement;
    m_replaceList += cmd;
}

void ChangeSet::move(int pos, int length, int to)
{
    Q_ASSERT(!hasOverlap(pos, length));

    Move cmd;
    cmd.pos = pos;
    cmd.length = length;
    cmd.to = to;
    m_moveList += cmd;
}

void ChangeSet::doReplace(const Replace &replace)
{
    int diff = replace.replacement.size() - replace.length;
    {
        QMutableListIterator<Replace> i(m_replaceList);
        while (i.hasNext()) {
            Replace &c = i.next();
            if (replace.pos < c.pos)
                c.pos += diff;
            else if (replace.pos + replace.length < c.pos + c.length)
                c.length += diff;
        }
    }
    {
        QMutableListIterator<Move> i(m_moveList);
        while (i.hasNext()) {
            Move &c = i.next();
            if (replace.pos < c.pos)
                c.pos += diff;
            else if (replace.pos + replace.length < c.pos + c.length)
                c.length += diff;

            if (replace.pos < c.to)
                c.to += diff;
        }
    }

    if (string) {
        string->replace(replace.pos, replace.length, replace.replacement);
    } else if (cursor) {
        cursor->setPosition(replace.pos);
        cursor->setPosition(replace.pos + replace.length, QTextCursor::KeepAnchor);
        cursor->insertText(replace.replacement);
    }
}

void ChangeSet::doMove(const Move &move)
{
    QString text;
    if (string) {
        text = string->mid(move.pos, move.length);
    } else if (cursor) {
        cursor->setPosition(move.pos);
        cursor->setPosition(move.pos + move.length, QTextCursor::KeepAnchor);
        text = cursor->selectedText();
    }

    Replace cut;
    cut.pos = move.pos;
    cut.length = move.length;
    Replace paste;
    paste.pos = move.to;
    paste.length = 0;
    paste.replacement = text;

    m_replaceList.append(cut);
    m_replaceList.append(paste);

    Replace cmd;
    while (!m_replaceList.isEmpty()) {
        cmd = m_replaceList.first();
        m_replaceList.removeFirst();
        doReplace(cmd);
    }
}

void ChangeSet::write(QString *s)
{
    string = s;
    write_helper();
    string = 0;
}

void ChangeSet::write(QTextCursor *textCursor)
{
    cursor = textCursor;
    write_helper();
    cursor = 0;
}

void ChangeSet::write_helper()
{
    if (cursor)
        cursor->beginEditBlock();
    {
        Replace cmd;
        while (!m_replaceList.isEmpty()) {
            cmd = m_replaceList.first();
            m_replaceList.removeFirst();
            doReplace(cmd);
        }
    }
    {
        Move cmd;
        while (!m_moveList.isEmpty()) {
            cmd = m_moveList.first();
            m_moveList.removeFirst();
            doMove(cmd);
        }
    }
    if (cursor)
        cursor->endEditBlock();
}

} // end namespace Utils
