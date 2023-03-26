// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveitem.h"
#include "treeitem.h"

#include <QItemSelectionModel>

namespace QmlDesigner {

class TreeItem;
class NodeTreeItem;
class PropertyTreeItem;

class SelectionModel : public QItemSelectionModel
{
    Q_OBJECT

signals:
    void curvesSelected();

public:
    SelectionModel(QAbstractItemModel *model = nullptr);

    void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;

    bool isSelected(TreeItem *item) const;

    std::vector<TreeItem::Path> selectedPaths() const;

    std::vector<CurveItem *> selectedCurveItems() const;

    std::vector<TreeItem *> selectedTreeItems() const;

    std::vector<NodeTreeItem *> selectedNodeItems() const;

    std::vector<PropertyTreeItem *> selectedPropertyItems() const;

    void selectPaths(const std::vector<TreeItem::Path> &selection);

private:
    void changeSelection(const QItemSelection &selected, const QItemSelection &deselected);
};

} // End namespace QmlDesigner.
