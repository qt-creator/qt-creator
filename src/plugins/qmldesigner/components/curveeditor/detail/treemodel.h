// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "treeitem.h"

#include <QAbstractItemModel>

#include <vector>

namespace QmlDesigner {

class GraphicsView;
class TreeView;
class CurveItem;
class SelectionModel;

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    static bool isTextColumn(const QModelIndex &index);

    static bool isLockedColumn(const QModelIndex &index);

    static bool isPinnedColumn(const QModelIndex &index);

    static TreeItem *treeItem(const QModelIndex &index);

    static NodeTreeItem *nodeItem(const QModelIndex &index);

    static PropertyTreeItem *propertyItem(const QModelIndex &index);

    static CurveItem *curveItem(const QModelIndex &index);

    static CurveItem *curveItem(TreeItem *item);

    TreeModel(QObject *parent = nullptr);

    ~TreeModel() override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    bool isSelected(TreeItem *item) const;

    std::vector<CurveItem *> selectedCurves() const;

    QModelIndex indexOf(const TreeItem::Path &path) const;

    TreeItem *find(unsigned int id);

    TreeItem *find(const QString &id);

    void setTreeView(TreeView *view);

    void setGraphicsView(GraphicsView *view);

protected:
    TreeView *treeView() const;

    GraphicsView *graphicsView() const;

    SelectionModel *selectionModel() const;

    void initialize();

    TreeItem *root();

    QModelIndex findIdx(const QString &name, const QModelIndex &parent) const;

private:
    GraphicsView *m_view;

    TreeView *m_tree;

    TreeItem *m_root;
};

} // End namespace QmlDesigner.
