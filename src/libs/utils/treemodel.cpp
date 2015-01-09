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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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
    : m_parent(0), m_displays(0), m_lazy(false), m_populated(false),
      m_flags(Qt::ItemIsEnabled|Qt::ItemIsSelectable)
{
}

TreeItem::TreeItem(const QStringList &displays)
    : m_parent(0), m_displays(new QStringList(displays)), m_lazy(false), m_populated(false),
      m_flags(Qt::ItemIsEnabled|Qt::ItemIsSelectable)
{
}

TreeItem::~TreeItem()
{
    clear();
    delete m_displays;
}

TreeItem *TreeItem::child(int pos) const
{
    ensurePopulated();
    QTC_ASSERT(pos >= 0, return 0);
    return pos < m_children.size() ? m_children.at(pos) : 0;
}

bool TreeItem::isLazy() const
{
    return m_lazy;
}

int TreeItem::columnCount() const
{
    return m_displays ? m_displays->size() : 1;
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
    if (role == Qt::DisplayRole && m_displays && column >= 0 && column < m_displays->size())
        return m_displays->at(column);
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

TreeItem *TreeItem::lastChild() const
{
    return m_children.isEmpty() ? 0 : m_children.last();
}

int TreeItem::level() const
{
    int l = 0;
    for (TreeItem *item = this->parent(); item; item = item->parent())
        ++l;
    return l;
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
        return m_root->columnCount();
    if (idx.column() > 0)
        return 0;
    return itemFromIndex(idx)->columnCount();
}

QVariant TreeModel::data(const QModelIndex &idx, int role) const
{
    TreeItem *item = itemFromIndex(idx);
    return item ? item->data(idx.column(), role) : QVariant();
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal)
        return m_root->data(section, role);
    return QVariant();
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

void TreeModel::setRootItem(TreeItem *item)
{
    delete m_root;
    m_root = item;
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
    return item ? indexFromItemHelper(item, m_root, QModelIndex()) : QModelIndex();
}

void TreeModel::appendItem(TreeItem *parent, TreeItem *item)
{
    QTC_ASSERT(item, return);
    QTC_ASSERT(parent, return);

    QModelIndex idx = indexFromItem(parent);
    const int n = parent->rowCount();
    beginInsertRows(idx, n, n);
    parent->appendChild(item);
    endInsertRows();
}

void TreeModel::updateItem(TreeItem *item)
{
    QModelIndex idx = indexFromItem(item);
    dataChanged(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), item->columnCount() - 1));
}

UntypedTreeLevelItems TreeModel::untypedLevelItems(int level, TreeItem *start) const
{
    if (start == 0)
        start = m_root;
    return UntypedTreeLevelItems(start, level);
}

UntypedTreeLevelItems TreeModel::untypedLevelItems(TreeItem *start) const
{
    return UntypedTreeLevelItems(start, 1);
}

void TreeModel::removeItem(TreeItem *item)
{
    QTC_ASSERT(item, return);
    TreeItem *parent = item->parent();
    QTC_ASSERT(parent, return);
    int pos = parent->m_children.indexOf(item);
    QTC_ASSERT(pos != -1, return);

    QModelIndex idx = indexFromItem(parent);
    beginRemoveRows(idx, pos, pos);
    item->m_parent = 0;
    parent->m_children.removeAt(pos);
    endRemoveRows();
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

//
// TreeLevelItems
//

UntypedTreeLevelItems::UntypedTreeLevelItems(TreeItem *item, int level)
    : m_item(item), m_level(level)
{}

UntypedTreeLevelItems::const_iterator::const_iterator(TreeItem *base, int level)
    : m_level(level)
{
    QTC_ASSERT(level > 0, return);
    if (base) {
        // "begin()"
        m_depth = 0;
        // Level x: The item m_item[x] is the m_pos[x]'th child of its
        // parent, out of m_size[x].
        m_pos[0] = 0;
        m_size[0] = 1;
        m_item[0] = base;
        goDown();
    } else {
        // "end()"
        m_depth = -1;
    }
}


// Result is either an item of the target level, or 'end'.
void UntypedTreeLevelItems::const_iterator::goDown()
{
    QTC_ASSERT(m_depth != -1, return);
    QTC_ASSERT(m_depth < m_level, return);
    do {
        TreeItem *curr = m_item[m_depth];
        int size = curr->rowCount();
        if (size == 0) {
            // This is a dead end not reaching to the desired level.
            goUpNextDown();
            return;
        }
        ++m_depth;
        m_size[m_depth] = size;
        m_pos[m_depth] = 0;
        m_item[m_depth] = curr->child(0);
    } while (m_depth < m_level);
    // Did not reach the required level? Set to end().
    if (m_depth != m_level)
        m_depth = -1;
}

void UntypedTreeLevelItems::const_iterator::goUpNextDown()
{
    // Go up until we can move sidewards.
    do {
        --m_depth;
        if (m_depth < 0)
            return; // Solid end.
    } while (++m_pos[m_depth] >= m_size[m_depth]);
    m_item[m_depth] = m_item[m_depth - 1]->child(m_pos[m_depth]);
    goDown();
}

bool UntypedTreeLevelItems::const_iterator::operator==(UntypedTreeLevelItems::const_iterator other) const
{
    if (m_depth != other.m_depth)
        return false;
    for (int i = 0; i <= m_depth; ++i)
        if (m_item[i] != other.m_item[i])
            return false;
    return true;
}

void UntypedTreeLevelItems::const_iterator::operator++()
{
    QTC_ASSERT(m_depth == m_level, return);

    int pos = ++m_pos[m_depth];
    if (pos < m_size[m_depth])
        m_item[m_depth] = m_item[m_depth - 1]->child(pos);
    else
        goUpNextDown();
}

UntypedTreeLevelItems::const_iterator UntypedTreeLevelItems::begin() const
{
    return const_iterator(m_item, m_level);
}

UntypedTreeLevelItems::const_iterator UntypedTreeLevelItems::end() const
{
    return const_iterator(0, m_level);
}

} // namespace Utils
