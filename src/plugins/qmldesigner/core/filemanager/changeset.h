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

public:
    ChangeSet();

    bool isEmpty() const;

    QList<EditOp> operationList() const;

    void clear();

    bool replace(int pos, int length, const QString &replacement);
    bool move(int pos, int length, int to);
    bool insert(int pos, const QString &text);
    bool remove(int pos, int length);
    bool flip(int pos1, int length1, int pos2, int length2);
    bool copy(int pos, int length, int to);

    bool hadErrors();

    void apply(QString *s);
    void apply(QTextCursor *textCursor);

private:
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
