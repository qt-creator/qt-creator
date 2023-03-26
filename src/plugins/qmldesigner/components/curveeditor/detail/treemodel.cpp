// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "treemodel.h"
#include "curveitem.h"
#include "detail/graphicsview.h"
#include "treeitem.h"
#include "treeview.h"

#include <QIcon>

namespace QmlDesigner {

bool TreeModel::isTextColumn(const QModelIndex &index)
{
    return index.column() == 0;
}

bool TreeModel::isLockedColumn(const QModelIndex &index)
{
    return index.column() == 1;
}

bool TreeModel::isPinnedColumn(const QModelIndex &index)
{
    return index.column() == 2;
}

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
        citem->setComponent(pti->component());
        citem->setLocked(pti->locked() || item->implicitlyLocked());
        citem->setPinned(pti->pinned() || item->implicitlyPinned());
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

int TreeModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
{
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

TreeView *TreeModel::treeView() const
{
    return m_tree;
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

bool TreeModel::isSelected(TreeItem *item) const
{
    if (auto *sm = selectionModel())
        return sm->isSelected(item);

    return false;
}

void addCurvesFromItem(TreeItem *item, std::vector<CurveItem *> &curves)
{
    if (auto *pitem = item->asPropertyItem()) {
        if (auto *curveItem = TreeModel::curveItem(pitem))
            curves.push_back(curveItem);
    } else if (auto *nitem = item->asNodeItem()) {
        for (auto *child : nitem->children())
            addCurvesFromItem(child, curves);
    }
}

std::vector<CurveItem *> TreeModel::selectedCurves() const
{
    std::vector<CurveItem *> curves;
    const auto ids = selectionModel()->selectedIndexes();
    for (auto &&index : ids) {
        if (auto *treeItem = TreeModel::treeItem(index))
            addCurvesFromItem(treeItem, curves);
    }
    return curves;
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

TreeItem *TreeModel::find(const QString &id)
{
    return m_root->find(id);
}

} // End namespace QmlDesigner.
