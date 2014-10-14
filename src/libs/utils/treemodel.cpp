/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "treemodel.h"
#include "qtcassert.h"

/*!
    \class Utils::TreeModel

    \brief The TreeModel class is a convienience base class for models
    to use in a QTreeView.
*/

namespace Utils {

//
// TreeItem
//
TreeItem::TreeItem()
    : m_parent(0), m_lazy(false), m_populated(false),
      m_flags(Qt::ItemIsEnabled|Qt::ItemIsSelectable)
{
}

TreeItem::~TreeItem()
{
    clear();
}

TreeItem *TreeItem::child(int pos) const
{
    ensurePopulated();
    QTC_ASSERT(pos >= 0, return 0);
    QTC_ASSERT(pos < m_children.size(), return 0);
    return m_children.at(pos);
}

bool TreeItem::isLazy() const
{
    return m_lazy;
}

int TreeItem::columnCount() const
{
    return 1;
}

int TreeItem::rowCount() const
{
    ensurePopulated();
    return m_children.size();
}

void TreeItem::populate()
{
}

QVariant TreeItem::data(int column, int role) const
{
    Q_UNUSED(column);
    Q_UNUSED(role);
    return QVariant();
}

Qt::ItemFlags TreeItem::flags(int column) const
{
    Q_UNUSED(column);
    return m_flags;
}

void TreeItem::prependChild(TreeItem *item)
{
    QTC_CHECK(!item->parent());
    item->m_parent = this;
    m_children.prepend(item);
}

void TreeItem::appendChild(TreeItem *item)
{
    QTC_CHECK(!item->parent());
    item->m_parent = this;
    m_children.append(item);
}

void TreeItem::setLazy(bool on)
{
    m_lazy = on;
}

void TreeItem::setFlags(Qt::ItemFlags flags)
{
    m_flags = flags;
}

void TreeItem::clear()
{
    while (m_children.size()) {
        TreeItem *item = m_children.takeLast();
        item->m_parent = 0;
        delete item;
    }
}

void TreeItem::ensurePopulated() const
{
    if (!m_populated) {
        if (isLazy())
            const_cast<TreeItem *>(this)->populate();
        m_populated = true;
    }
}

//
// TreeModel
//
TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent), m_root(new TreeItem)
{
}

TreeModel::~TreeModel()
{
    delete m_root;
}

QModelIndex TreeModel::parent(const QModelIndex &idx) const
{
    checkIndex(idx);
    if (!idx.isValid())
        return QModelIndex();

    const TreeItem *item = itemFromIndex(idx);
    const TreeItem *parent = item->parent();
    if (!parent || parent == m_root)
        return QModelIndex();

    const TreeItem *grandparent = parent->parent();
    if (!grandparent)
        return QModelIndex();

    for (int i = 0, n = grandparent->rowCount(); i < n; ++i)
        if (grandparent->child(i) == parent)
            return createIndex(i, 0, (void*) parent);

    return QModelIndex();
}

int TreeModel::rowCount(const QModelIndex &idx) const
{
    checkIndex(idx);
    if (!idx.isValid())
        return m_root->rowCount();
    if (idx.column() > 0)
        return 0;
    return itemFromIndex(idx)->rowCount();
}

int TreeModel::columnCount(const QModelIndex &idx) const
{
    checkIndex(idx);
    if (!idx.isValid())
        return m_root->rowCount();
    if (idx.column() > 0)
        return 0;
    return itemFromIndex(idx)->columnCount();
}

QVariant TreeModel::data(const QModelIndex &idx, int role) const
{
    TreeItem *item = itemFromIndex(idx);
    return item ? item->data(idx.column(), role) : QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &idx) const
{
    TreeItem *item = itemFromIndex(idx);
    return item ? item->flags(idx.column())
                : (Qt::ItemIsEnabled|Qt::ItemIsSelectable);
}

TreeItem *TreeModel::rootItem() const
{
    return m_root;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    checkIndex(parent);
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const TreeItem *item = itemFromIndex(parent);
    QTC_ASSERT(item, return QModelIndex());
    if (row >= item->rowCount())
        return QModelIndex();
    return createIndex(row, column, (void*)(item->child(row)));
}

TreeItem *TreeModel::itemFromIndex(const QModelIndex &idx) const
{
    checkIndex(idx);
    TreeItem *item = idx.isValid() ? static_cast<TreeItem*>(idx.internalPointer()) : m_root;
//    CHECK(checkItem(item));
    return item;
}

QModelIndex TreeModel::indexFromItem(const TreeItem *item) const
{
//    CHECK(checkItem(item));
    return indexFromItemHelper(item, m_root, QModelIndex());
}

QModelIndex TreeModel::indexFromItemHelper(const TreeItem *needle,
    TreeItem *parentItem, const QModelIndex &parentIndex) const
{
    checkIndex(parentIndex);
    if (needle == parentItem)
        return parentIndex;
    for (int i = parentItem->rowCount(); --i >= 0; ) {
        TreeItem *childItem = parentItem->child(i);
        QModelIndex childIndex = index(i, 0, parentIndex);
        QModelIndex idx = indexFromItemHelper(needle, childItem, childIndex);
        checkIndex(idx);
        if (idx.isValid())
            return idx;
    }
    return QModelIndex();
}

void TreeModel::checkIndex(const QModelIndex &index) const
{
    if (index.isValid()) {
        QTC_CHECK(index.model() == this);
    } else {
        QTC_CHECK(index.model() == 0);
    }
}

} // namespace Utils
