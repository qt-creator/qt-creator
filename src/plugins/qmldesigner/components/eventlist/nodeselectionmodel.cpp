/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/
#include "nodeselectionmodel.h"
#include "eventlist.h"
#include "nodelistview.h"
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace QmlDesigner {

NodeSelectionModel::NodeSelectionModel(QAbstractItemModel *model)
    : QItemSelectionModel(model)
{}

void NodeSelectionModel::select(const QItemSelection &selection,
                                QItemSelectionModel::SelectionFlags command)
{
    QItemSelectionModel::select(selection, command);

    for (auto &&modelIndex : selection.indexes()) {
        if (modelIndex.column() == NodeListModel::idColumn) {
            int id = modelIndex.data(NodeListModel::internalIdRole).toInt();
            EventList::selectNode(id);
            emit nodeSelected(modelIndex.data(NodeListModel::eventIdsRole).toStringList());
        }
    }
}

QList<int> NodeSelectionModel::selectedNodes() const
{
    QList<int> out;
    for (auto index : selectedRows())
        out.push_back(index.data(NodeListModel::internalIdRole).toInt());

    return out;
}

void NodeSelectionModel::selectNode(int nodeId)
{
    if (nodeId < 0) {
        clearSelection();
        return;
    }

    if (auto *model = qobject_cast<QAbstractItemModel *>(this->model())) {
        auto start = model->index(0, 0);
        auto indexes = model->match(start,
                                    NodeListModel::internalIdRole,
                                    QString::number(nodeId),
                                    1,
                                    Qt::MatchExactly);

        for (auto index : indexes)
            QItemSelectionModel::select(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    }
}

void NodeSelectionModel::storeSelection()
{
    if (const auto *proxyModel = qobject_cast<const QSortFilterProxyModel *>(model())) {
        if (hasSelection())
            m_stored = proxyModel->mapSelectionToSource(selection());
    }
}

void NodeSelectionModel::reselect()
{
    if (const auto *proxyModel = qobject_cast<const QSortFilterProxyModel *>(model()))
        select(proxyModel->mapSelectionFromSource(m_stored),
               QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

} // namespace QmlDesigner.
