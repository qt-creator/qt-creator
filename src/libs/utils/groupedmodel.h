// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "algorithm.h"
#include "utils_global.h"

#include <QAbstractTableModel>
#include <QObject>
#include <QTreeView>

#include <utility>

namespace Utils {

class QTCREATOR_UTILS_EXPORT GroupedModel : public QAbstractTableModel
{
public:
    explicit GroupedModel(QObject *parent = nullptr);
    ~GroupedModel() override;

    QAbstractItemModel *groupedDisplayModel() const;

    QModelIndex mapToSource(const QModelIndex &index) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    int itemCount() const;
    bool isAdded(int row) const;
    bool isRemoved(int row) const;
    bool isChanged(int row) const;
    bool isDirty(int row) const;
    int defaultRow() const;
    bool isDefault(int row) const;
    void setVolatileDefaultRow(int row);
    void markRemoved(int row);
    void removeItem(int row);
    void setChanged(int row, bool changed);
    void notifyRowChanged(int row);
    void notifyAllRowsChanged();

    virtual void apply();
    virtual void cancel();

    const QVariantList &variants() const;
    void setVariants(const QVariantList &items);

    QModelIndex appendVariant(const QVariant &item);

    QVariant volatileVariant(int row) const;
    void setVolatileVariant(int row, const QVariant &item);

    QModelIndex appendVolatileVariant(const QVariant &item);
    const QVariantList &volatileVariants() const;

    using Filter = std::function<bool(int)>;
    void setFilters(const QString &defaultTitle,
                    const QList<std::pair<QString, Filter>> &sections = {});
    void setExtraFilter(const Filter &filter);
    void setHeader(const QStringList &header);

protected:
    void setDefaultRow(int row);
    void setShowDefault(bool on);
    void setDefaultAffectsDirty(bool on);
    virtual void onDefaultRowChanged(int oldRow, int newRow);

private:
    virtual QVariant variantData(const QVariant &item, int column, int role) const = 0;

    QVariantList m_variants;
    QStringList m_header;
    int m_columnCount = 0;

    QVariantList m_volatileVariants;
    QList<bool> m_added;
    QList<bool> m_removed;
    QList<bool> m_changed;

    QList<bool> m_volatileDefaultFlag;
    QList<bool> m_defaultFlag;
    bool m_showDefault = false;
    bool m_defaultAffectsDirty = false;

    class DisplayModel;
    DisplayModel *m_displayModel = nullptr;
};

template<class ItemType>
class TypedGroupedModel : public GroupedModel
{
public:
    using GroupedModel::GroupedModel;

    ItemType item(int row) const
    {
        return fromVariant(volatileVariant(row));
    }

    QModelIndex appendItem(const ItemType &item)
    {
        return appendVariant(toVariant(item));
    }

    QList<ItemType> items() const
    {
        return Utils::transform(variants(), &fromVariant);
    }

    void setItems(const QList<ItemType> &items)
    {
        setVariants(Utils::transform(items, &toVariant));
    }

    QList<ItemType> volatileItems() const
    {
        return Utils::transform(volatileVariants(), &fromVariant);
    }

    void setVolatileItem(int row, const ItemType &item)
    {
        setVolatileVariant(row, toVariant(item));
    }

    QModelIndex appendVolatileItem(const ItemType &item)
    {
        return appendVolatileVariant(toVariant(item));
    }

protected:
    static ItemType fromVariant(const QVariant &v)
    {
        return v.template value<ItemType>();
    }

    static QVariant toVariant(const ItemType &item)
    {
        return QVariant::fromValue<ItemType>(item);
    }
};

class QTCREATOR_UTILS_EXPORT GroupedView : public QObject
{
    Q_OBJECT

public:
    explicit GroupedView(GroupedModel &model);

    QTreeView &view();

    int currentRow() const;
    void selectRow(int row);
    void scrollToRow(int row);

signals:
    void currentRowChanged(int oldRow, int newRow);

private:
    GroupedModel &m_model;
    QTreeView m_view;
};

} // namespace Utils
