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
            this, &CollectionDetailsSortFilterModel::updateRowCountChanges);
    connect(this, &CollectionDetailsSortFilterModel::rowsRemoved,
            this, &CollectionDetailsSortFilterModel::updateRowCountChanges);
    connect(this, &CollectionDetailsSortFilterModel::modelReset,
            this, &CollectionDetailsSortFilterModel::updateRowCountChanges);

    setDynamicSortFilter(true);
}

void CollectionDetailsSortFilterModel::setSourceModel(CollectionDetailsModel *model)
{
    m_source = model;
    Super::setSourceModel(model);
    connect(m_source, &CollectionDetailsModel::selectedColumnChanged,
            this, &CollectionDetailsSortFilterModel::updateSelectedColumn);

    connect(m_source, &CollectionDetailsModel::selectedRowChanged,
            this, &CollectionDetailsSortFilterModel::updateSelectedRow);
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

void CollectionDetailsSortFilterModel::deselectAll()
{
    QTC_ASSERT(m_source, return);
    m_source->deselectAll();
}

CollectionDetailsSortFilterModel::~CollectionDetailsSortFilterModel() = default;

bool CollectionDetailsSortFilterModel::filterAcceptsRow(int sourceRow,
                                                        const QModelIndex &sourceParent) const
{
    QTC_ASSERT(m_source, return false);
    QModelIndex sourceIndex(m_source->index(sourceRow, 0, sourceParent));
    return sourceIndex.isValid();
}

bool CollectionDetailsSortFilterModel::lessThan(const QModelIndex &sourceleft,
                                                const QModelIndex &sourceRight) const
{
    QTC_ASSERT(m_source, return false);

    if (sourceleft.column() == sourceRight.column()) {
        int column = sourceleft.column();
        CollectionDetails::DataType columnType = m_source->propertyDataType(column);
        return CollectionEditorUtils::variantIslessThan(sourceleft.data(),
                                                        sourceRight.data(),
                                                        columnType);
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

void CollectionDetailsSortFilterModel::updateSelectedRow()
{
    const int upToDateSelectedRow = selectedRow();
    if (m_selectedRow == upToDateSelectedRow)
        return;

    const int rows = rowCount();
    const int columns = columnCount();
    const int previousRow = m_selectedRow;

    m_selectedRow = upToDateSelectedRow;
    emit this->selectedRowChanged(m_selectedRow);

    auto notifySelectedDataChanged = [this, rows, columns](int notifyingRow) {
        if (notifyingRow > -1 && notifyingRow < rows && columns) {
            emit dataChanged(index(notifyingRow, 0),
                             index(notifyingRow, columns - 1),
                             {CollectionDetailsModel::SelectedRole});
        }
    };

    notifySelectedDataChanged(previousRow);
    notifySelectedDataChanged(m_selectedRow);
}

void CollectionDetailsSortFilterModel::updateSelectedColumn()
{
    const int upToDateSelectedColumn = selectedColumn();
    if (m_selectedColumn == upToDateSelectedColumn)
        return;

    const int rows = rowCount();
    const int columns = columnCount();
    const int previousColumn = m_selectedColumn;

    m_selectedColumn = upToDateSelectedColumn;
    emit this->selectedColumnChanged(m_selectedColumn);

    auto notifySelectedDataChanged = [this, rows, columns](int notifyingCol) {
        if (notifyingCol > -1 && notifyingCol < columns && rows) {
            emit dataChanged(index(0, notifyingCol),
                             index(rows - 1, notifyingCol),
                             {CollectionDetailsModel::SelectedRole});
        }
    };

    notifySelectedDataChanged(previousColumn);
    notifySelectedDataChanged(m_selectedColumn);
}

void CollectionDetailsSortFilterModel::updateRowCountChanges()
{
    updateEmpty();
    updateSelectedRow();
    invalidate();
}

} // namespace QmlDesigner
