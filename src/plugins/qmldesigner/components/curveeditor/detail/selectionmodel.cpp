// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectionmodel.h"
#include "curveitem.h"
#include "treemodel.h"

namespace QmlDesigner {

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

bool SelectionModel::isSelected(TreeItem *item) const
{
    for (auto *i : selectedTreeItems())
        if (i->id() == item->id())
            return true;

    return false;
}

std::vector<TreeItem::Path> SelectionModel::selectedPaths() const
{
    std::vector<TreeItem::Path> out;
    for (auto *item : selectedTreeItems())
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

void SelectionModel::changeSelection([[maybe_unused]] const QItemSelection &selected,
                                     [[maybe_unused]] const QItemSelection &deselected)
{
    emit curvesSelected();
}

} // End namespace QmlDesigner.
