/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "utils_global.h"

#include <QAbstractItemModel>

#include <functional>

namespace Utils {

class TreeModel;

class QTCREATOR_UTILS_EXPORT TreeItem
{
public:
    TreeItem();
    virtual ~TreeItem();

    virtual TreeItem *parent() const { return m_parent; }
    virtual TreeItem *child(int pos) const;
    virtual int rowCount() const;

    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &data, int role);
    virtual Qt::ItemFlags flags(int column) const;

    virtual bool hasChildren() const;
    virtual bool canFetchMore() const;
    virtual void fetchMore() {}

    void prependChild(TreeItem *item);
    void appendChild(TreeItem *item);
    void insertChild(int pos, TreeItem *item);
    void removeChildren();
    void sortChildren(const std::function<bool(const TreeItem *, const TreeItem *)> &cmp);
    void update();
    void updateColumn(int column);
    void expand();
    TreeItem *firstChild() const;
    TreeItem *lastChild() const;
    int level() const;

    void setFlags(Qt::ItemFlags flags);
    int childCount() const { return m_children.size(); }
    TreeItem *childAt(int index) const { return m_children.at(index); }
    QVector<TreeItem *> children() const { return m_children; }
    QModelIndex index() const;

    TreeModel *model() const { return m_model; }

    template <class T, class Predicate>
    void forSelectedChildren(const Predicate &pred) const {
        foreach (TreeItem *item, m_children) {
            if (pred(static_cast<T>(item)))
                item->forSelectedChildren<T, Predicate>(pred);
        }
    }

    template <class T, typename Predicate>
    void forAllChildren(const Predicate &pred) const {
        foreach (TreeItem *item, m_children) {
            pred(static_cast<T>(item));
            item->forAllChildren<T, Predicate>(pred);
        }
    }

    // Levels are 1-based: Child at Level 1 is an immediate child.
    template <class T, typename Predicate>
    void forFirstLevelChildren(Predicate pred) const {
        foreach (TreeItem *item, m_children)
            pred(static_cast<T>(item));
    }

    template <class T, typename Predicate>
    void forSecondLevelChildren(Predicate pred) const {
        foreach (TreeItem *item1, m_children)
            foreach (TreeItem *item2, item1->m_children)
                pred(static_cast<T>(item2));
    }

    template <class T, typename Predicate>
    T findFirstLevelChild(Predicate pred) const {
        foreach (TreeItem *item, m_children)
            if (pred(static_cast<T>(item)))
                return static_cast<T>(item);
        return 0;
    }

    template <class T, typename Predicate>
    T findSecondLevelChild(Predicate pred) const {
        foreach (TreeItem *item1, m_children)
            foreach (TreeItem *item2, item1->children())
                if (pred(static_cast<T>(item2)))
                    return static_cast<T>(item2);
        return 0;
    }

    template <class T, typename Predicate>
    T findAnyChild(Predicate pred) const {
        foreach (TreeItem *item, m_children) {
            if (pred(static_cast<T>(item)))
                return static_cast<T>(item);
            if (T found = item->findAnyChild<T>(pred))
                return found;
        }
        return 0;
    }

private:
    TreeItem(const TreeItem &) Q_DECL_EQ_DELETE;
    void operator=(const TreeItem &) Q_DECL_EQ_DELETE;

    void clear();
    void propagateModel(TreeModel *m);

    TreeItem *m_parent; // Not owned.
    TreeModel *m_model; // Not owned.
    Qt::ItemFlags m_flags;

 protected:
    QVector<TreeItem *> m_children; // Owned.
    friend class TreeModel;
};

// A TreeItem with children all of the same type.
template <class ChildType>
class TypedTreeItem : public TreeItem
{
public:
    ChildType *childAt(int index) const { return static_cast<ChildType *>(TreeItem::childAt(index)); }

    void sortChildren(const std::function<bool(const ChildType *, const ChildType *)> &lessThan)
    {
        return TreeItem::sortChildren([lessThan](const TreeItem *a, const TreeItem *b) {
            return lessThan(static_cast<const ChildType *>(a), static_cast<const ChildType *>(b));
        });
    }

    template <typename Predicate>
    void forAllChildren(const Predicate &pred) const {
        return TreeItem::forAllChildren<ChildType *, Predicate>(pred);
    }

    template <typename Predicate>
    ChildType *findFirstLevelChild(Predicate pred) const {
        return TreeItem::findFirstLevelChild<ChildType *, Predicate>(pred);
    }
};

class QTCREATOR_UTILS_EXPORT StaticTreeItem : public TreeItem
{
public:
    StaticTreeItem(const QStringList &displays);
    StaticTreeItem(const QString &display);

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;

private:
    QStringList m_displays;
};

// A general purpose multi-level model where each item can have its
// own (TreeItem-derived) type.
class QTCREATOR_UTILS_EXPORT TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TreeModel(QObject *parent = 0);
    explicit TreeModel(TreeItem *root, QObject *parent = 0);
    ~TreeModel() override;

    void setHeader(const QStringList &displays);
    void setHeaderToolTip(const QStringList &tips);
    void clear();

    TreeItem *rootItem() const;
    void setRootItem(TreeItem *item);
    TreeItem *itemForIndex(const QModelIndex &) const;
    QModelIndex indexForItem(const TreeItem *needle) const;

    int topLevelItemCount() const;
    int rowCount(const QModelIndex &idx = QModelIndex()) const override;
    int columnCount(const QModelIndex &idx) const override;

    bool setData(const QModelIndex &idx, const QVariant &data, int role) override;
    QVariant data(const QModelIndex &idx, int role) const override;
    QModelIndex index(int, int, const QModelIndex &idx = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &idx) const override;
    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool hasChildren(const QModelIndex &idx) const override;

    bool canFetchMore(const QModelIndex &idx) const override;
    void fetchMore(const QModelIndex &idx) override;

    TreeItem *takeItem(TreeItem *item); // item is not destroyed.

signals:
    void requestExpansion(QModelIndex);

protected:
    friend class TreeItem;

    TreeItem *m_root; // Owned.
    QStringList m_header;
    QStringList m_headerToolTip;
    int m_columnCount;
};

// A multi-level model with uniform types per level.
// All items below second level have to have identitical types.
template <class FirstLevelItem,
          class SecondLevelItem = FirstLevelItem,
          class RootItem = TreeItem>
class LeveledTreeModel : public TreeModel
{
public:
    explicit LeveledTreeModel(QObject *parent = 0) : TreeModel(parent) {}
    explicit LeveledTreeModel(RootItem *root, QObject *parent = 0) : TreeModel(root, parent) {}

    template <class Predicate>
    void forFirstLevelItems(const Predicate &pred) const {
        m_root->forFirstLevelChildren<FirstLevelItem *>(pred);
    }

    template <class Predicate>
    void forSecondLevelItems(const Predicate &pred) const {
        m_root->forSecondLevelChildren<SecondLevelItem *>(pred);
    }

    template <class Predicate>
    FirstLevelItem *findFirstLevelItem(const Predicate &pred) const {
        return m_root->findFirstLevelChild<FirstLevelItem *>(pred);
    }

    template <class Predicate>
    SecondLevelItem *findSecondLevelItem(const Predicate &pred) const {
        return m_root->findSecondLevelChild<SecondLevelItem *>(pred);
    }

    RootItem *rootItem() const {
        return static_cast<RootItem *>(TreeModel::rootItem());
    }


    FirstLevelItem *firstLevelItemForIndex(const QModelIndex &idx) const {
        TreeItem *item = TreeModel::itemForIndex(idx);
        return item && item->level() == 1 ? static_cast<FirstLevelItem *>(item) : 0;
    }

    SecondLevelItem *secondLevelItemForIndex(const QModelIndex &idx) const {
        TreeItem *item = TreeModel::itemForIndex(idx);
        return item && item->level() == 2 ? static_cast<SecondLevelItem *>(item) : 0;
    }
};

// A model where all non-root nodes are the same.
template <class ItemType>
class UniformTreeModel : public LeveledTreeModel<ItemType, ItemType, ItemType>
{
public:
    using BaseType = LeveledTreeModel<ItemType, ItemType, ItemType>;

    explicit UniformTreeModel(QObject *parent = 0) : BaseType(parent) {}

    ItemType *nonRootItemForIndex(const QModelIndex &idx) const {
        TreeItem *item = TreeModel::itemForIndex(idx);
        return item && item->parent() ? static_cast<ItemType *>(item) : 0;
    }

    template <class Predicate>
    ItemType *findNonRooItem(const Predicate &pred) const {
        TreeItem *root = this->rootItem();
        return root->findAnyChild<ItemType *>(pred);
    }

    template <class Predicate>
    void forSelectedItems(const Predicate &pred) const {
        TreeItem *root = this->rootItem();
        root->forSelectedChildren<ItemType *, Predicate>(pred);
    }

    template <class Predicate>
    void forAllItems(const Predicate &pred) const {
        TreeItem *root = this->rootItem();
        root->forAllChildren<ItemType *, Predicate>(pred);
    }

    ItemType *itemForIndex(const QModelIndex &idx) const {
        return static_cast<ItemType *>(BaseType::itemForIndex(idx));
    }
};

} // namespace Utils
