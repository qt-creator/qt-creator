// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "constants.h"
#include "todoicons.h"

#include <utils/filepath.h>

#include <QMetaType>
#include <QString>
#include <QColor>

namespace Todo {
namespace Internal {

class TodoItem
{
public:
    QString text;
    Utils::FilePath file;
    int line = -1;
    IconType iconType = IconType::Todo;
    QColor color;
};

class TodoItemSortPredicate
{
public:
    explicit TodoItemSortPredicate(Constants::OutputColumnIndex columnIndex, Qt::SortOrder order) :
        m_columnIndex(columnIndex),
        m_order(order)
    {}

    inline bool operator()(const TodoItem &t1, const TodoItem &t2)
    {
        if (m_order == Qt::AscendingOrder)
            return lessThan(t1, t2);
        else
            return lessThan(t2, t1);
    }

    inline bool lessThan(const TodoItem &t1, const TodoItem &t2)
    {
        switch (m_columnIndex) {
            case Constants::OUTPUT_COLUMN_TEXT:
                return t1.text < t2.text;

            case Constants::OUTPUT_COLUMN_LINE:
                return t1.line < t2.line;

            case Constants::OUTPUT_COLUMN_FILE:
                return t1.file < t2.file;

            default:
                Q_ASSERT(false);
                return false;
        }
    }

private:
    Constants::OutputColumnIndex m_columnIndex;
    Qt::SortOrder m_order;
};

} // namespace Internal
} // namespace Todo

Q_DECLARE_METATYPE(Todo::Internal::TodoItem)
