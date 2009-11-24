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

#ifndef CHANGESET_H
#define CHANGESET_H

#include "utils_global.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtGui/QTextCursor>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ChangeSet
{
public:
    struct Replace {
        Replace(): pos(0), length(0) {}

        int pos;
        int length;
        QString replacement;
    };

    struct Move {
        Move(): pos(0), length(0), to(0) {}

        int pos;
        int length;
        int to;
    };

public:
    ChangeSet();

    bool isEmpty() const;

    QList<Replace> replaceList() const;
    QList<Move> moveList() const; // ### TODO: merge with replaceList

    void clear();

    void replace(int pos, int length, const QString &replacement);
    void move(int pos, int length, int to);

    void write(QString *s);
    void write(QTextCursor *textCursor);

private:
    bool hasOverlap(int pos, int length);
    bool hasMoveInto(int pos, int length);

    void doReplace(const Replace &replace);
    void doMove(const Move &move);

    void write_helper();

private:
    QString *m_string;
    QTextCursor *m_cursor;
    QList<Replace> m_replaceList;
    QList<Move> m_moveList;
};

} // namespace Utils

#endif // CHANGESET_H
