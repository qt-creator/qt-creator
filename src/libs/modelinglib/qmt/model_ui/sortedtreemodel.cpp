// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

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
