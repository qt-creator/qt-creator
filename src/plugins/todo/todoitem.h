/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "constants.h"
#include "todoicons.h"

#include <utils/fileutils.h>

#include <QMetaType>
#include <QString>
#include <QColor>

namespace Todo {
namespace Internal {

class TodoItem
{
public:
    TodoItem() : line(-1) {}

    QString text;
    Utils::FileName file;
    int line;
    IconType iconType;
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
