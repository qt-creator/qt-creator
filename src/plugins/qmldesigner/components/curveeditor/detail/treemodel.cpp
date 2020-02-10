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

#include "treemodel.h"
#include "curveitem.h"
#include "detail/graphicsview.h"
#include "treeitem.h"
#include "treeview.h"

#include <QIcon>

namespace DesignTools {

TreeItem *TreeModel::treeItem(const QModelIndex &index)
{
    if (index.isValid() && index.column() == 0)
        return static_cast<TreeItem *>(index.internalPointer());

    return nullptr;
}

NodeTreeItem *TreeModel::nodeItem(const QModelIndex &index)
{
    if (auto *ti = treeItem(index))
        return ti->asNodeItem();

    return nullptr;
}

PropertyTreeItem *TreeModel::propertyItem(const QModelIndex &index)
{
    if (auto *ti = treeItem(index))
        return ti->asPropertyItem();

    return nullptr;
}

CurveItem *TreeModel::curveItem(const QModelIndex &index)
{
    if (auto *ti = treeItem(index))
        return curveItem(ti);

    return nullptr;
}

CurveItem *TreeModel::curveItem(TreeItem *item)
{
    if (auto *pti = item->asPropertyItem()) {
        auto *citem = new CurveItem(pti->id(), pti->curve());
        citem->setValueType(pti->valueType());
        citem->setComponent(pti->component());
        citem->setLocked(pti->locked());
        citem->setPinned(pti->pinned());
        return citem;
    }

    return nullptr;
}

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_view(nullptr)
    , m_root(new TreeItem("Root"))
{}

TreeModel::~TreeModel()
{
    if (m_root) {
        delete m_root;
        m_root = nullptr;
    }

    m_view = nullptr;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());

    if (role == Qt::DecorationRole && index.column() == 0)
        return item->icon();

    if (role != Qt::DisplayRole)
        return QVariant();

    return item->data(index.column());
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return m_root->headerData(section);

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem = m_root;

    if (parent.isValid())
        parentItem = static_cast<TreeItem *>(parent.internalPointer());

    if (TreeItem *childItem = parentItem->child(row))
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem *>(index.internalPointer());

    if (TreeItem *parentItem = childItem->parent()) {
        if (parentItem == m_root)
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }
    return QModelIndex();
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    TreeItem *parentItem = m_root;

    if (parent.isValid())
        parentItem = static_cast<TreeItem *>(parent.internalPointer());

    return parentItem->rowCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_root->columnCount();
}

void TreeModel::setTreeView(TreeView *view)
{
    m_tree = view;
}

void TreeModel::setGraphicsView(GraphicsView *view)
{
    m_view = view;
}

GraphicsView *TreeModel::graphicsView() const
{
    return m_view;
}

SelectionModel *TreeModel::selectionModel() const
{
    if (m_tree)
        return m_tree->selectionModel();
    return nullptr;
}

QModelIndex TreeModel::findIdx(const QString &name, const QModelIndex &parent) const
{
    for (int i = 0; i < rowCount(parent); ++i) {
        QModelIndex idx = index(i, 0, parent);
        if (idx.isValid()) {
            TreeItem *item = static_cast<TreeItem *>(idx.internalPointer());
            if (item->name() == name)
                return idx;
        }
    }
    return QModelIndex();
}

QModelIndex TreeModel::indexOf(const TreeItem::Path &path) const
{
    QModelIndex parent;
    for (size_t i = 0; i < path.size(); ++i) {
        QModelIndex idx = findIdx(path[i], parent);
        if (idx.isValid())
            parent = idx;
    }

    return parent;
}

void TreeModel::initialize()
{
    if (m_root)
        delete m_root;

    m_root = new TreeItem("Root");
}

TreeItem *TreeModel::root()
{
    return m_root;
}

TreeItem *TreeModel::find(unsigned int id)
{
    return m_root->find(id);
}

} // End namespace DesignTools.
