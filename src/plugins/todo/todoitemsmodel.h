/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef TODOITEMSMODEL_H
#define TODOITEMSMODEL_H

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

public slots:
    void todoItemsListUpdated();

private:
    QList<TodoItem> *m_todoItemsList;
    Constants::OutputColumnIndex m_currentSortColumn;
    Qt::SortOrder m_currentSortOrder;
};

}
}

#endif // TODOITEMSMODEL_H
