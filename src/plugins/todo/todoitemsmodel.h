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

#include "todoitem.h"

#include <QAbstractTableModel>
#include <QList>

namespace Todo {
namespace Internal {

class TodoItemsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TodoItemsModel(QObject *parent = 0);

    void setTodoItemsList(QList<TodoItem> *list);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    void todoItemsListUpdated();

private:
    QList<TodoItem> *m_todoItemsList;
    Constants::OutputColumnIndex m_currentSortColumn;
    Qt::SortOrder m_currentSortOrder;
};

}
}
