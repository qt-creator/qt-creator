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

#ifndef UTILS_TREEMODEL_H
#define UTILS_TREEMODEL_H

#include "utils_global.h"
#include "algorithm.h"

#include <QAbstractItemModel>

#include <functional>
#include <iterator>

namespace Utils {

class QTCREATOR_UTILS_EXPORT TreeItem
{
public:
    TreeItem();
    explicit TreeItem(const QStringList &displays);
    virtual ~TreeItem();

    virtual TreeItem *parent() const { return m_parent; }
    virtual TreeItem *child(int pos) const;
    virtual bool isLazy() const;
    virtual int columnCount() const;
    virtual int rowCount() const;
    virtual void populate();

    virtual QVariant data(int column, int role) const;
    virtual Qt::ItemFlags flags(int column) const;

    void prependChild(TreeItem *item);
    void appendChild(TreeItem *item);
    TreeItem *lastChild() const;
    int level() const;

    void setLazy(bool on);
    void setPopulated(bool on);
    void setFlags(Qt::ItemFlags flags);
    QVector<TreeItem *> children() const { return m_children; }

private:
    TreeItem(const TreeItem &) Q_DECL_EQ_DELETE;
    void operator=(const TreeItem &) Q_DECL_EQ_DELETE;

    void clear();
    void ensurePopulated() const;

    TreeItem *m_parent; // Not owned.
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
        void operator++();

        bool operator==(const_iterator other) const;
        bool operator!=(const_iterator other) const { return !operator==(other); }

    private:
        void goDown();
        void goUpNextDown();

        int m_level;
        int m_depth;
        TreeItem *m_item[8];
        int m_pos[8];
        int m_size[8];
    };

    const_iterator begin() const;
    const_iterator end() const;

private:
    TreeItem *m_item;
    int m_level;
};

template <class T>
class QTCREATOR_UTILS_EXPORT TreeLevelItems
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
public:
    explicit TreeModel(QObject *parent = 0);
    virtual ~TreeModel();

    int rowCount(const QModelIndex &idx = QModelIndex()) const;
    int columnCount(const QModelIndex &idx) const;

    QVariant data(const QModelIndex &idx, int role) const;
    QModelIndex index(int, int, const QModelIndex &idx) const;
    QModelIndex parent(const QModelIndex &idx) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    TreeItem *rootItem() const;
    void setRootItem(TreeItem *item);
    TreeItem *itemFromIndex(const QModelIndex &) const;
    QModelIndex indexFromItem(const TreeItem *needle) const;

    void appendItem(TreeItem *parent, TreeItem *item);
    void removeItem(TreeItem *item); // item is not destroyed.
    void updateItem(TreeItem *item); // call to trigger dataChanged

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

private:
    QModelIndex indexFromItemHelper(const TreeItem *needle,
        TreeItem *parentItem, const QModelIndex &parentIndex) const;

    TreeItem *m_root; // Owned.
};

} // namespace Utils

#endif // UTILS_TREEMODEL_H
