// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"
#include "qmt/model_ui/modeltreefilterdata.h"

#include <QSortFilterProxyModel>
#include <QTimer>

namespace qmt {

class TreeModel;

class QMT_EXPORT SortedTreeModel : public QSortFilterProxyModel
{
public:
    explicit SortedTreeModel(QObject *parent = nullptr);
    ~SortedTreeModel() override;

    TreeModel *treeModel() const { return m_treeModel; }
    void setTreeModel(TreeModel *treeModel);

    void setModelTreeViewData(const ModelTreeViewData &viewData);
    void setModelTreeFilterData(const ModelTreeFilterData &filterData);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void onTreeModelRowsInserted(const QModelIndex &parent, int start, int end);
    void onDataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &);
    void onDelayedSortTimeout();

    void startDelayedSortTimer();

    TreeModel *m_treeModel = nullptr;
    ModelTreeViewData m_modelTreeViewData;
    ModelTreeFilterData m_modelTreeViewFilterData;
    QTimer m_delayedSortTimer;
};

} // namespace qmt
