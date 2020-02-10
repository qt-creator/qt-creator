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

#pragma once

#include "treeitem.h"

#include <QAbstractItemModel>

#include <vector>

namespace DesignTools {

class GraphicsView;
class TreeView;
class TreeItem;
class CurveItem;
class PropertyTreeItem;
class SelectionModel;

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
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

    QModelIndex indexOf(const TreeItem::Path &path) const;

    void setTreeView(TreeView *view);

    void setGraphicsView(GraphicsView *view);

protected:
    GraphicsView *graphicsView() const;

    SelectionModel *selectionModel() const;

    void initialize();

    TreeItem *root();

    TreeItem *find(unsigned int id);

    QModelIndex findIdx(const QString &name, const QModelIndex &parent) const;

private:
    GraphicsView *m_view;

    TreeView *m_tree;

    TreeItem *m_root;
};

} // End namespace DesignTools.
