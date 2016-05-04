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

    void walkTree(TreeItemVisitor *visitor);
    void walkTree(std::function<void(TreeItem *)> f);

    // Levels are 1-based: Child at Level 1 is an immediate child.
    template <class T, typename Function>
    void forEachChildAtLevel(int n, Function func) {
        foreach (auto item, m_children) {
            if (n == 1)
                func(static_cast<T>(item));
            else
                item->forEachChildAtLevel<T>(n - 1, func);
        }
    }

    template <class T, typename Function>
    void forEachChild(Function func) const {
        forEachChildAtLevel<T>(1, func);
    }

    template <class T, typename Predicate>
    T findChildAtLevel(int n, Predicate func) const {
        if (n == 1) {
            foreach (auto item, m_children)
                if (func(static_cast<T>(item)))
                    return static_cast<T>(item);
        } else {
            foreach (auto item, m_children)
                if (T found = item->findChildAtLevel<T>(n - 1, func))
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
    QVector<TreeItem *> m_children; // Owned.
    QStringList *m_displays;
    Qt::ItemFlags m_flags;

    friend class TreeModel;
};

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

    template <class T, typename Function>
    void forEachItemAtLevel(int n, Function func) const {
        m_root->forEachChildAtLevel<T>(n, func);
    }

    template <class T, typename Predicate>
    T findItemAtLevel(int n, Predicate func) const {
        return m_root->findChildAtLevel<T>(n, func);
    }

    TreeItem *takeItem(TreeItem *item); // item is not destroyed.

signals:
    void requestExpansion(QModelIndex);

private:
    friend class TreeItem;

    TreeItem *m_root; // Owned.
    QStringList m_header;
    QStringList m_headerToolTip;
    int m_columnCount;
};

} // namespace Utils
