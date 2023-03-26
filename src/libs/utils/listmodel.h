// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "treemodel.h"

namespace Utils {

template <class ChildType>
class BaseListModel : public TreeModel<TypedTreeItem<ChildType>, ChildType>
{
public:
    using BaseModel = TreeModel<TypedTreeItem<ChildType>, ChildType>;
    using BaseModel::rootItem;

    explicit BaseListModel(QObject *parent = nullptr) : BaseModel(parent) {}

    int itemCount() const { return rootItem()->childCount(); }
    ChildType *itemAt(int row) const { return rootItem()->childAt(row); }

    void appendItem(ChildType *item) { rootItem()->appendChild(item); }

    template <class Predicate>
    void forItems(const Predicate &pred) const
    {
        rootItem()->forFirstLevelChildren(pred);
    }

    template <class Predicate>
    ChildType *findItem(const Predicate &pred) const
    {
        return rootItem()->findFirstLevelChild(pred);
    }

    void sortItems(const std::function<bool(const ChildType *, const ChildType *)> &lessThan)
    {
        return rootItem()->sortChildren([lessThan](const TreeItem *a, const TreeItem *b) {
            return lessThan(static_cast<const ChildType *>(a), static_cast<const ChildType *>(b));
        });
    }

    int indexOf(const ChildType *item) const { return rootItem()->indexOf(item); }

    void clear() { rootItem()->removeChildren(); }

    auto begin() const { return rootItem()->begin(); }
    auto end() const { return rootItem()->end(); }
};

template <class ItemData>
class ListItem : public TreeItem
{
public:
    ItemData itemData;
};

template <class ItemData>
class ListModel : public BaseListModel<ListItem<ItemData>>
{
public:
    using ChildType = ListItem<ItemData>;
    using BaseModel = BaseListModel<ChildType>;

    explicit ListModel(QObject *parent = nullptr) : BaseModel(parent) {}

    const ItemData &dataAt(int row) const
    {
        static const ItemData dummyData = {};
        auto item = BaseModel::itemAt(row);
        return item ? item->itemData : dummyData;
    }

    ChildType *findItemByData(const std::function<bool(const ItemData &)> &pred) const
    {
        return BaseModel::rootItem()->findFirstLevelChild([pred](ChildType *child) {
            return pred(child->itemData);
        });
    }

    void destroyItems(const std::function<bool(const ItemData &)> &pred)
    {
        QList<ChildType *> toDestroy;
        BaseModel::rootItem()->forFirstLevelChildren([pred, &toDestroy](ChildType *item) {
            if (pred(item->itemData))
                toDestroy.append(item);
        });
        for (ChildType *item : toDestroy)
            this->destroyItem(item);
    }

    ItemData *findData(const std::function<bool(const ItemData &)> &pred) const
    {
        ChildType *item = findItemByData(pred);
        return item ? &item->itemData : nullptr;
    }

    QModelIndex findIndex(const std::function<bool(const ItemData &)> &pred) const
    {
        ChildType *item = findItemByData(pred);
        return item ? BaseTreeModel::indexForItem(item) : QModelIndex();
    }

    QList<ItemData> allData() const
    {
        QList<ItemData> res;
        BaseModel::rootItem()->forFirstLevelChildren([&res](ChildType *child) {
            res.append(child->itemData);
        });
        return res;
    }

    QList<ItemData> allData(const std::function<bool(const ItemData &)> &pred) const
    {
        QList<ItemData> res;
        BaseModel::rootItem()->forFirstLevelChildren([pred, &res](ChildType *item) {
            if (pred(item->itemData))
                res.append(item->itemData);
        });
        return res;
    }

    void setAllData(const QList<ItemData> &items)
    {
        BaseModel::rootItem()->removeChildren();
        for (const ItemData &data : items)
            appendItem(data);
    }

    void forAllData(const std::function<void(ItemData &)> &func) const
    {
        BaseModel::rootItem()->forFirstLevelChildren([func](ChildType *child) {
            func(child->itemData);
        });
    }

    ChildType *appendItem(const ItemData &data)
    {
        auto item = new ChildType;
        item->itemData = data;
        BaseModel::rootItem()->appendChild(item);
        return item;
    }

    QVariant data(const QModelIndex &idx, int role) const override
    {
        TreeItem *item = BaseModel::itemForIndex(idx);
        if (item && item->parent() == BaseModel::rootItem())
            return itemData(static_cast<ChildType *>(item)->itemData, idx.column(), role);
        return {};
    }

    Qt::ItemFlags flags(const QModelIndex &idx) const override
    {
        TreeItem *item = BaseModel::itemForIndex(idx);
        if (item && item->parent() == BaseModel::rootItem())
            return itemFlags(static_cast<ChildType *>(item)->itemData, idx.column());
        return {};
    }

    using QAbstractItemModel::itemData;
    virtual QVariant itemData(const ItemData &idata, int column, int role) const
    {
        if (m_dataAccessor)
            return m_dataAccessor(idata, column, role);
        return {};
    }

    virtual Qt::ItemFlags itemFlags(const ItemData &idata, int column) const
    {
        if (m_flagsAccessor)
            return m_flagsAccessor(idata, column);
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    void setDataAccessor(const std::function<QVariant(const ItemData &, int, int)> &accessor)
    {
        m_dataAccessor = accessor;
    }

    void setFlagsAccessor(const std::function<Qt::ItemFlags(const ItemData &, int)> &accessor)
    {
        m_flagsAccessor = accessor;
    }

private:
    std::function<QVariant(const ItemData &, int, int)> m_dataAccessor;
    std::function<Qt::ItemFlags(const ItemData &, int)> m_flagsAccessor;
};

} // namespace Utils
