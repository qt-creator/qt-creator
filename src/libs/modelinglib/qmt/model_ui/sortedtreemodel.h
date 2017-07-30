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

#pragma once

#include <QSortFilterProxyModel>
#include "qmt/infrastructure/qmt_global.h"

#include <QTimer>

namespace qmt {

class TreeModel;

class QMT_EXPORT SortedTreeModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit SortedTreeModel(QObject *parent = nullptr);
    ~SortedTreeModel() override;

    TreeModel *treeModel() const { return m_treeModel; }
    void setTreeModel(TreeModel *treeModel);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void onTreeModelRowsInserted(const QModelIndex &parent, int start, int end);
    void onDataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &);
    void onDelayedSortTimeout();

    void startDelayedSortTimer();

    TreeModel *m_treeModel = nullptr;
    QTimer m_delayedSortTimer;
};

} // namespace qmt
