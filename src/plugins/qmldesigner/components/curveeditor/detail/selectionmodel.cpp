/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "selectionmodel.h"
#include "curveitem.h"
#include "treemodel.h"

namespace DesignTools {

SelectionModel::SelectionModel(QAbstractItemModel *model)
    : QItemSelectionModel(model)
{
    connect(this, &QItemSelectionModel::selectionChanged, this, &SelectionModel::changeSelection);
}

void SelectionModel::select(const QItemSelection &selection,
                            QItemSelectionModel::SelectionFlags command)
{
    for (auto &&index : selection.indexes()) {
        if (index.column() == 0) {
            QItemSelectionModel::select(selection, command);
            return;
        }
    }
}

std::vector<TreeItem::Path> SelectionModel::selectedPaths() const
{
    std::vector<TreeItem::Path> out;
    for (auto &&item : selectedTreeItems())
        out.push_back(item->path());
    return out;
}

std::vector<CurveItem *> SelectionModel::selectedCurveItems() const
{
    std::vector<CurveItem *> items;
    const auto rows = selectedRows(0);
    for (auto &&index : rows) {
        if (auto *curveItem = TreeModel::curveItem(index))
            items.push_back(curveItem);
    }
    return items;
}

std::vector<TreeItem *> SelectionModel::selectedTreeItems() const
{
    std::vector<TreeItem *> items;
    const auto rows = selectedRows(0);
    for (auto &&index : rows) {
        if (auto *treeItem = TreeModel::treeItem(index))
            items.push_back(treeItem);
    }
    return items;
}

std::vector<NodeTreeItem *> SelectionModel::selectedNodeItems() const
{
    std::vector<NodeTreeItem *> items;
    const auto rows = selectedRows(0);
    for (auto &&index : rows) {
        if (auto *ni = TreeModel::nodeItem(index))
            items.push_back(ni);
    }
    return items;
}

std::vector<PropertyTreeItem *> SelectionModel::selectedPropertyItems() const
{
    std::vector<PropertyTreeItem *> items;
    const auto rows = selectedRows(0);
    for (auto &&index : rows) {
        if (auto *pi = TreeModel::propertyItem(index))
            items.push_back(pi);
    }
    return items;
}

void SelectionModel::selectPaths(const std::vector<TreeItem::Path> &selection)
{
    for (auto &&path : selection) {
        if (auto *treeModel = qobject_cast<TreeModel *>(model())) {
            QModelIndex left = treeModel->indexOf(path);
            QModelIndex right = left.siblingAtColumn(2);
            if (left.isValid() && right.isValid()) {
                auto is = QItemSelection(left, right);
                QItemSelectionModel::select(is, QItemSelectionModel::Select);
            }
        }
    }
}

void SelectionModel::changeSelection(const QItemSelection &selected,
                                     const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    std::vector<CurveItem *> curves;
    const auto ids = selectedIndexes();
    for (auto &&index : ids) {
        if (auto *curveItem = TreeModel::curveItem(index))
            curves.push_back(curveItem);
    }

    emit curvesSelected(curves);
}

} // End namespace DesignTools.
