/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef UTILS_TREEMODEL_H
#define UTILS_TREEMODEL_H

#include "utils_global.h"

#include "algorithm.h"
#include "qtcassert.h"

#include <QAbstractItemModel>

#include <functional>
#include <iterator>

namespace Utils {

class TreeItem;
class TreeModel;

class QTCREATOR_UTILS_EXPORT TreeItemVisitor
{
public:
    TreeItemVisitor() : m_level(0) {}
    virtual ~TreeItemVisitor() {}

    virtual bool preVisit(TreeItem *) { return true; }
    virtual void visit(TreeItem *) {}
    virtual void postVisit(TreeItem *) {}

    int level() const { return m_level; }

private:
    friend class TreeItem;
    int m_level;
};

class QTCREATOR_UTILS_EXPORT TreeItem
{
public:
    TreeItem();
    explicit TreeItem(const QStringList &displays, int flags = Qt::ItemIsEnabled);
    virtual ~TreeItem();

    virtual TreeItem *parent() const { return m_parent; }
    virtual TreeItem *child(int pos) const;
    virtual bool isLazy() const;
    virtual int rowCount() const;
    virtual void populate();

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

    void setLazy(bool on);
    void setPopulated(bool on);
    void setFlags(Qt::ItemFlags flags);
    QVector<TreeItem *> children() const { return m_children; }
    QModelIndex index() const;

    TreeModel *model() const { return m_model; }
    void setModel(TreeModel *model);

    void walkTree(TreeItemVisitor *visitor);
    void walkTree(std::function<void(TreeItem *)> f);

private:
    TreeItem(const TreeItem &) Q_DECL_EQ_DELETE;
    void operator=(const TreeItem &) Q_DECL_EQ_DELETE;

    void clear();
    void ensurePopulated() const;
    void propagateModel(TreeModel *m);

    TreeItem *m_parent; // Not owned.
    TreeModel *m_model; // Not owned.
    QVector<TreeItem *> m_children; // Owned.
    QStringList *m_displays;
    bool m_lazy;
    mutable bool m_populated;
    Qt::ItemFlags m_flags;

    friend class TreeModel;
};

class QTCREATOR_UTILS_EXPORT UntypedTreeLevelItems
{
public:
    enum { MaxSearchDepth = 12 }; // FIXME.
    explicit UntypedTreeLevelItems(TreeItem *item, int level = 1);

    typedef TreeItem *value_type;

    class const_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef TreeItem *value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const value_type *pointer;
        typedef const value_type &reference;

        const_iterator(TreeItem *base, int level);

        TreeItem *operator*() { return m_item[m_depth]; }

        void operator++()
        {
            QTC_ASSERT(m_depth == m_level, return);

            int pos = ++m_pos[m_depth];
            if (pos < m_size[m_depth])
                m_item[m_depth] = m_item[m_depth - 1]->child(pos);
            else
                goUpNextDown();
        }

        bool operator==(const const_iterator &other) const
        {
            if (m_depth != other.m_depth)
                return false;
            for (int i = 0; i <= m_depth; ++i)
                if (m_item[i] != other.m_item[i])
                    return false;
            return true;
        }

        bool operator!=(const const_iterator &other) const
        {
            return !operator==(other);
        }

    private:
        // Result is either an item of the target level, or 'end'.
        void goDown()
        {
            QTC_ASSERT(m_depth != -1, return);
            QTC_ASSERT(m_depth < m_level, return);
            do {
                TreeItem *curr = m_item[m_depth];
                ++m_depth;
                int size = curr->rowCount();
                if (size == 0) {
                    // This is a dead end not reaching to the desired level.
                    goUpNextDown();
                    return;
                }
                m_size[m_depth] = size;
                m_pos[m_depth] = 0;
                m_item[m_depth] = curr->child(0);
            } while (m_depth < m_level);
            // Did not reach the required level? Set to end().
            if (m_depth != m_level)
                m_depth = -1;
        }
        void goUpNextDown()
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

        int m_level;
        int m_depth;
        TreeItem *m_item[MaxSearchDepth];
        int m_pos[MaxSearchDepth];
        int m_size[MaxSearchDepth];
    };

    const_iterator begin() const;
    const_iterator end() const;

private:
    TreeItem *m_item;
    int m_level;
};

template <class T>
class TreeLevelItems
{
public:
    typedef T value_type;

    explicit TreeLevelItems(const UntypedTreeLevelItems &items) : m_items(items) {}

    struct const_iterator : public UntypedTreeLevelItems::const_iterator
    {
        typedef std::forward_iterator_tag iterator_category;
        typedef T value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const value_type *pointer;
        typedef const value_type &reference;

        const_iterator(UntypedTreeLevelItems::const_iterator it) : UntypedTreeLevelItems::const_iterator(it) {}
        T operator*() { return static_cast<T>(UntypedTreeLevelItems::const_iterator::operator*()); }
    };

    const_iterator begin() const { return const_iterator(m_items.begin()); }
    const_iterator end() const { return const_iterator(m_items.end()); }

private:
    UntypedTreeLevelItems m_items;
};

class QTCREATOR_UTILS_EXPORT TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TreeModel(QObject *parent = 0);
    explicit TreeModel(TreeItem *root, QObject *parent = 0);
    virtual ~TreeModel();

    int rowCount(const QModelIndex &idx = QModelIndex()) const;
    int columnCount(const QModelIndex &idx) const;

    bool setData(const QModelIndex &idx, const QVariant &data, int role);
    QVariant data(const QModelIndex &idx, int role) const;
    QModelIndex index(int, int, const QModelIndex &idx) const;
    QModelIndex parent(const QModelIndex &idx) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool hasChildren(const QModelIndex &idx) const;

    bool canFetchMore(const QModelIndex &idx) const;
    void fetchMore(const QModelIndex &idx);

    TreeItem *rootItem() const;
    void setRootItem(TreeItem *item);
    TreeItem *itemFromIndex(const QModelIndex &) const;
    QModelIndex indexFromItem(const TreeItem *needle) const;
    void removeItems();

    void setHeader(const QStringList &displays);
    void setColumnCount(int columnCount);

    UntypedTreeLevelItems untypedLevelItems(int level = 0, TreeItem *start = 0) const;
    UntypedTreeLevelItems untypedLevelItems(TreeItem *start) const;

    template <class T>
    TreeLevelItems<T> treeLevelItems(int level, TreeItem *start = 0) const
    {
        return TreeLevelItems<T>(untypedLevelItems(level, start));
    }

    template <class T>
    TreeLevelItems<T> treeLevelItems(TreeItem *start) const
    {
        return TreeLevelItems<T>(untypedLevelItems(start));
    }

    template <class T>
    T findItemAtLevel(int level, std::function<bool(T)> f, TreeItem *start = 0) const
    {
        return Utils::findOrDefault(treeLevelItems<T>(level, start), f);
    }

    void removeItem(TreeItem *item); // item is not destroyed.

signals:
    void requestExpansion(QModelIndex);

private:
    friend class TreeItem;

    TreeItem *m_root; // Owned.
    QStringList m_header;
    int m_columnCount;
};

} // namespace Utils

#endif // UTILS_TREEMODEL_H
