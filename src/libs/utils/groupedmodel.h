// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "algorithm.h"
#include "utils_global.h"

#include <QAbstractTableModel>
#include <QObject>
#include <QPushButton>
#include <QTreeView>

#include <utility>

namespace Utils {

class QTCREATOR_UTILS_EXPORT GroupedModel : public QAbstractTableModel
{
public:
    GroupedModel();
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
    bool isDirty() const;
    int defaultRow() const;
    bool isDefault(int row) const;
    void setVolatileDefaultRow(int row);
    virtual void markRemoved(int row);
    virtual int cloneRow(int row) { Q_UNUSED(row); return -1; }
    void removeItem(int row);
    void setChanged(int row, bool changed);
    void notifyRowChanged(int row);
    void notifyAllRowsChanged();

    virtual void apply();
    virtual void cancel();

    const QVariantList &variants() const;
    void setVariants(const QVariantList &items);

    int appendVariant(const QVariant &item);

    QVariant volatileVariant(int row) const;
    void setVolatileVariant(int row, const QVariant &item);
    void resetVariant(int row, const QVariant &item);

    int appendVolatileVariant(const QVariant &item);
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
    bool isOriginalDefault(int row) const;

private:
    virtual QVariant variantData(int row, int column, int role) const = 0;

    void reassignDefaultIfNeeded(int row);

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

    int appendItem(const ItemType &item)
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

    void resetItem(int row, const ItemType &item)
    {
        resetVariant(row, toVariant(item));
    }

    int appendVolatileItem(const ItemType &item)
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

    QPushButton &removeButton();
    QPushButton &cloneButton();
    QPushButton &makeDefaultButton();

    void setCanRemoveRow(std::function<bool(int)> predicate);
    void setCanCloneRow(std::function<bool(int)> predicate);

    int currentRow() const;
    void selectRow(int row);
    void scrollToRow(int row);

    void removeCurrent();
    void cloneCurrent();

    void updateButtons();

signals:
    void currentRowChanged(int oldRow, int newRow);
    void currentRemoved();
    void currentCloned();

private:
    GroupedModel &m_model;
    QTreeView m_view;
    QPushButton m_removeButton;
    QPushButton m_cloneButton;
    QPushButton m_makeDefaultButton;
    QVariant m_savedVariant;
    int m_removedDefaultRow = -1;
    std::function<bool(int)> m_canRemove;
    std::function<bool(int)> m_canClone;
};

} // namespace Utils
