/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "sortedtreemodel.h"

#include "treemodel.h"

namespace qmt {

SortedTreeModel::SortedTreeModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);
    setSortCaseSensitivity(Qt::CaseInsensitive);

    m_delayedSortTimer.setSingleShot(true);
    connect(&m_delayedSortTimer, &QTimer::timeout, this, &SortedTreeModel::onDelayedSortTimeout);

    connect(this, &QAbstractItemModel::rowsInserted,
            this, &SortedTreeModel::onTreeModelRowsInserted);
    connect(this, &QAbstractItemModel::dataChanged,
            this, &SortedTreeModel::onDataChanged);
}

SortedTreeModel::~SortedTreeModel()
{
}

void SortedTreeModel::setTreeModel(TreeModel *treeModel)
{
    m_treeModel = treeModel;
    setSourceModel(treeModel);
    startDelayedSortTimer();
}

bool SortedTreeModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    TreeModel::ItemType leftItemType = TreeModel::ItemType(sourceModel()->data(left, TreeModel::RoleItemType).toInt());
    TreeModel::ItemType rightItemType = TreeModel::ItemType(sourceModel()->data(right, TreeModel::RoleItemType).toInt());
    if (leftItemType < rightItemType) {
        return true;
    } else if (leftItemType > rightItemType) {
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

} // namespace qmt
