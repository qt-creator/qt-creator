// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todoitemsmodel.h"

#include "constants.h"
#include "todotr.h"

#include <utils/algorithm.h>
#include <utils/theme/theme.h>

#include <QIcon>

using namespace Utils;

namespace Todo {
namespace Internal {

TodoItemsModel::TodoItemsModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_currentSortColumn(Constants::OutputColumnIndex(0))
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

    if (role == Qt::ForegroundRole)
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
            return Tr::tr("Description");
        case Constants::OUTPUT_COLUMN_FILE:
            return Tr::tr("File");
        case Constants::OUTPUT_COLUMN_LINE:
            return Tr::tr("Line");
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
