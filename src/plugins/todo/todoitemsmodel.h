// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "todoitem.h"

#include <QAbstractTableModel>
#include <QList>

namespace Todo {
namespace Internal {

class TodoItemsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TodoItemsModel(QObject *parent = nullptr);

    void setTodoItemsList(QList<TodoItem> *list);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    void todoItemsListUpdated();

private:
    QList<TodoItem> *m_todoItemsList = nullptr;
    Constants::OutputColumnIndex m_currentSortColumn;
    Qt::SortOrder m_currentSortOrder = Qt::AscendingOrder;
};

}
}
