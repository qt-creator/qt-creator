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

#include "todoitemsmodel.h"
#include "constants.h"

#include <utils/algorithm.h>

#include <utils/theme/theme.h>

#include <QIcon>

using namespace Utils;

namespace Todo {
namespace Internal {

TodoItemsModel::TodoItemsModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_todoItemsList(0),
    m_currentSortColumn(Constants::OutputColumnIndex(0)),
    m_currentSortOrder(Qt::AscendingOrder)
{
}

void TodoItemsModel::setTodoItemsList(QList<TodoItem> *list)
{
    m_todoItemsList = list;
    todoItemsListUpdated();
}

int TodoItemsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    // There's only one level of hierarchy
    if (parent.isValid())
        return 0;

    if (!m_todoItemsList)
        return 0;

    return m_todoItemsList->count();
}

int TodoItemsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return Constants::OUTPUT_COLUMN_COUNT;
}

QVariant TodoItemsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TodoItem item = m_todoItemsList->at(index.row());

    if (role == Qt::TextColorRole)
        return item.color;

    switch (index.column()) {

        case Constants::OUTPUT_COLUMN_TEXT:
            switch (role) {
                case Qt::DisplayRole:
                    return item.text;
                case Qt::DecorationRole:
                    return icon(item.iconType);
            }
            break;

        case Constants::OUTPUT_COLUMN_FILE:
            if (role == Qt::DisplayRole)
                return item.file.toUserOutput();
            break;

        case Constants::OUTPUT_COLUMN_LINE:
            if (role == Qt::DisplayRole)
                return item.line;
            break;
    }

    return QVariant();
}

QVariant TodoItemsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
        case Constants::OUTPUT_COLUMN_TEXT:
            return tr(Constants::OUTPUT_COLUMN_TEXT_TITLE);

        case Constants::OUTPUT_COLUMN_FILE:
            return tr(Constants::OUTPUT_COLUMN_FILE_TITLE);

        case Constants::OUTPUT_COLUMN_LINE:
            return tr(Constants::OUTPUT_COLUMN_LINE_TITLE);

        default:
            return QVariant();
    }
}

void TodoItemsModel::sort(int column, Qt::SortOrder order)
{
    m_currentSortColumn = Constants::OutputColumnIndex(column);
    m_currentSortOrder = order;

    TodoItemSortPredicate predicate(m_currentSortColumn, m_currentSortOrder);
    emit layoutAboutToBeChanged();
    Utils::sort(*m_todoItemsList, predicate);
    emit layoutChanged();
}

void TodoItemsModel::todoItemsListUpdated()
{
    if (!m_todoItemsList)
        return;

    sort(m_currentSortColumn, m_currentSortOrder);
}

}
}
