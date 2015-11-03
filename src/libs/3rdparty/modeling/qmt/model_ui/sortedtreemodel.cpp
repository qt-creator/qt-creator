/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "sortedtreemodel.h"

#include "treemodel.h"


namespace qmt {

SortedTreeModel::SortedTreeModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);
    setSortCaseSensitivity(Qt::CaseInsensitive);

    m_delayedSortTimer.setSingleShot(true);
    connect(&m_delayedSortTimer, SIGNAL(timeout()), this, SLOT(onDelayedSortTimeout()));

    connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(onTreeModelRowsInserted(QModelIndex,int,int)));
    connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(onDataChanged(QModelIndex,QModelIndex,QVector<int>)));
}

SortedTreeModel::~SortedTreeModel()
{

}

void SortedTreeModel::setTreeModel(TreeModel *tree_model)
{
    m_treeModel = tree_model;
    setSourceModel(tree_model);
    startDelayedSortTimer();
}

bool SortedTreeModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    TreeModel::ItemType left_item_type = (TreeModel::ItemType) sourceModel()->data(left, TreeModel::ROLE_ITEM_TYPE).toInt();
    TreeModel::ItemType right_item_type = (TreeModel::ItemType) sourceModel()->data(right, TreeModel::ROLE_ITEM_TYPE).toInt();
    if (left_item_type < right_item_type) {
        return true;
    } else if (left_item_type > right_item_type) {
        return false;
    } else {
        QVariant l = sourceModel()->data(left);
        QVariant r = sourceModel()->data(right);
        switch (sortCaseSensitivity()) {
        case Qt::CaseInsensitive:
            return l.toString().compare(r.toString(), Qt::CaseInsensitive) < 0;
        case Qt::CaseSensitive:
            return l.toString() < r.toString();
        default:
            return l.toString() < r.toString();
        }
    }
}

void SortedTreeModel::onTreeModelRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);

    startDelayedSortTimer();
}

void SortedTreeModel::onDataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)
{
    startDelayedSortTimer();
}

void SortedTreeModel::onDelayedSortTimeout()
{
    sort(0, Qt::AscendingOrder);
}

void SortedTreeModel::startDelayedSortTimer()
{
    m_delayedSortTimer.start(1000);
}

}
