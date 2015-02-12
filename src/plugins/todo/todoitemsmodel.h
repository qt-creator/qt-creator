/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
