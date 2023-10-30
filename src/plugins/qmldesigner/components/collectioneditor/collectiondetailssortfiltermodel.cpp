// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectiondetailssortfiltermodel.h"

#include "collectiondetailsmodel.h"
#include "collectioneditorutils.h"

#include <utils/qtcassert.h>

namespace QmlDesigner {

CollectionDetailsSortFilterModel::CollectionDetailsSortFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &CollectionDetailsSortFilterModel::rowsInserted,
            this, &CollectionDetailsSortFilterModel::updateEmpty);
    connect(this, &CollectionDetailsSortFilterModel::rowsRemoved,
            this, &CollectionDetailsSortFilterModel::updateEmpty);
    connect(this, &CollectionDetailsSortFilterModel::modelReset,
            this, &CollectionDetailsSortFilterModel::updateEmpty);
}

void CollectionDetailsSortFilterModel::setSourceModel(CollectionDetailsModel *model)
{
    m_source = model;
    Super::setSourceModel(model);
    connect(m_source, &CollectionDetailsModel::selectedColumnChanged, this, [this](int sourceColumn) {
        emit selectedColumnChanged(mapFromSource(m_source->index(0, sourceColumn)).column());
    });

    connect(m_source, &CollectionDetailsModel::selectedRowChanged, this, [this](int sourceRow) {
        emit selectedRowChanged(mapFromSource(m_source->index(sourceRow, 0)).row());
    });
}

int CollectionDetailsSortFilterModel::selectedRow() const
{
    QTC_ASSERT(m_source, return -1);

    return mapFromSource(m_source->index(m_source->selectedRow(), 0)).row();
}

int CollectionDetailsSortFilterModel::selectedColumn() const
{
    QTC_ASSERT(m_source, return -1);

    return mapFromSource(m_source->index(0, m_source->selectedColumn())).column();
}

bool CollectionDetailsSortFilterModel::selectRow(int row)
{
    QTC_ASSERT(m_source, return false);

    return m_source->selectRow(mapToSource(index(row, 0)).row());
}

bool CollectionDetailsSortFilterModel::selectColumn(int column)
{
    QTC_ASSERT(m_source, return false);

    return m_source->selectColumn(mapToSource(index(0, column)).column());
}

CollectionDetailsSortFilterModel::~CollectionDetailsSortFilterModel() = default;

bool CollectionDetailsSortFilterModel::filterAcceptsRow(
    [[maybe_unused]] int sourceRow, [[maybe_unused]] const QModelIndex &sourceParent) const
{
    return true;
}

bool CollectionDetailsSortFilterModel::lessThan(const QModelIndex &sourceleft,
                                                const QModelIndex &sourceRight) const
{
    QTC_ASSERT(m_source, return false);

    if (sourceleft.column() == sourceRight.column()) {
        int column = sourceleft.column();
        CollectionDetails::DataType columnType = m_source->propertyDataType(column);
        return CollectionEditor::variantIslessThan(sourceleft.data(), sourceRight.data(), columnType);
    }

    return false;
}

void CollectionDetailsSortFilterModel::updateEmpty()
{
    bool newValue = rowCount() == 0;
    if (m_isEmpty != newValue) {
        m_isEmpty = newValue;
        emit isEmptyChanged(m_isEmpty);
    }
}

} // namespace QmlDesigner
