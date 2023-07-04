// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>
#include <QList>

QT_BEGIN_NAMESPACE
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

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

        EditOp() = default;
        EditOp(Type t): type(t) {}

        Type type = Unset;
        int pos1 = 0;
        int pos2 = 0;
        int length1 = 0;
        int length2 = 0;
        QString text;
    };

    struct Range {
        Range() = default;

        Range(int start, int end)
            : start(start), end(end) {}

        int start = 0;
        int end = 0;
    };

public:
    ChangeSet();
    ChangeSet(const QList<EditOp> &operations);

    bool isEmpty() const;

    QList<EditOp> operationList() const;

    void clear();

    bool replace(const Range &range, const QString &replacement);
    bool remove(const Range &range);
    bool move(const Range &range, int to);
    bool flip(const Range &range1, const Range &range2);
    bool copy(const Range &range, int to);
    bool replace(int start, int end, const QString &replacement);
    bool remove(int start, int end);
    bool move(int start, int end, int to);
    bool flip(int start1, int end1, int start2, int end2);
    bool copy(int start, int end, int to);
    bool insert(int pos, const QString &text);

    bool hadErrors() const;

    void apply(QString *s);
    void apply(QTextCursor *textCursor);
    void apply(QTextDocument *document);

private:
    // length-based API.
    bool replace_helper(int pos, int length, const QString &replacement);
    bool move_helper(int pos, int length, int to);
    bool remove_helper(int pos, int length);
    bool flip_helper(int pos1, int length1, int pos2, int length2);
    bool copy_helper(int pos, int length, int to);

    bool hasOverlap(int pos, int length) const;
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

using EditOperations = QList<ChangeSet::EditOp>;

inline bool operator<(const ChangeSet::Range &r1, const ChangeSet::Range &r2)
{
    return r1.start < r2.start;
}

} // namespace Utils
