/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CHANGESET_H
#define CHANGESET_H

#include "utils_global.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtGui/QTextCursor>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ChangeSet
{
public:
    struct EditOp {
        enum Type
        {
            Unset,
            Replace,
            Move,
            Insert,
            Remove,
            Flip,
            Copy
        };

        EditOp(): type(Unset), pos1(0), pos2(0), length1(0), length2(0) {}
        EditOp(Type t): type(t), pos1(0), pos2(0), length1(0), length2(0) {}

        Type type;
        int pos1;
        int pos2;
        int length1;
        int length2;
        QString text;
    };

    struct Range {
        Range()
            : start(0), end(0) {}

        Range(int start, int end)
            : start(start), end(end) {}

        int start;
        int end;
    };

public:
    ChangeSet();

    bool isEmpty() const;

    QList<EditOp> operationList() const;

    void clear();

    bool replace(const Range &range, const QString &replacement)
    { return replace(range.start, range.end, replacement); }

    bool remove(const Range &range)
    { return remove(range.start, range.end); }

    bool move(const Range &range, int to)
    { return move(range.start, range.end, to); }

    bool flip(const Range &range1, const Range &range2)
    { return flip(range1.start, range1.end, range2.start, range2.end); }

    bool copy(const Range &range, int to)
    { return copy(range.start, range.end, to); }


    bool replace(int start, int end, const QString &replacement)
    { return replace_helper(start, end - start, replacement); }

    bool remove(int start, int end)
    { return remove_helper(start, end - start); }

    bool move(int start, int end, int to)
    { return move_helper(start, end - start, to); }

    bool flip(int start1, int end1, int start2, int end2)
    { return flip_helper(start1, end1 - start1, start2, end2 - start2); }

    bool copy(int start, int end, int to)
    { return copy_helper(start, end - start, to); }

    bool insert(int pos, const QString &text);

    bool hadErrors();

    void apply(QString *s);
    void apply(QTextCursor *textCursor);

private:
    bool replace_helper(int pos, int length, const QString &replacement);
    bool move_helper(int pos, int length, int to);
    bool remove_helper(int pos, int length);
    bool flip_helper(int pos1, int length1, int pos2, int length2);
    bool copy_helper(int pos, int length, int to);

    bool hasOverlap(int pos, int length);
    QString textAt(int pos, int length);

    void doReplace(const EditOp &replace, QList<EditOp> *replaceList);
    void convertToReplace(const EditOp &op, QList<EditOp> *replaceList);

    void apply_helper();

private:
    QString *m_string;
    QTextCursor *m_cursor;

    QList<EditOp> m_operationList;
    bool m_error;
};

} // namespace Utils

#endif // CHANGESET_H
