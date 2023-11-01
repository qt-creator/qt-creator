// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindproxymodel.h"

#include "callgrinddatamodel.h"
#include "callgrindfunction.h"
#include "callgrindfunctioncall.h"
#include "callgrindparsedata.h"

#include <utils/qtcassert.h>

namespace Valgrind::Callgrind {

DataProxyModel::DataProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

const Function *DataProxyModel::filterFunction() const
{
    return m_function;
}

void DataProxyModel::setFilterBaseDir ( const QString &baseDir )
{
    if (m_baseDir == baseDir)
        return;

    m_baseDir = baseDir;
    invalidateFilter();
}

void DataProxyModel::setFilterFunction(const Function *function)
{
    if (m_function == function)
        return;

    const Function *previousFunction = m_function;
    m_function = function;
    invalidateFilter();
    emit filterFunctionChanged(previousFunction, function);
}

void DataProxyModel::setFilterMaximumRows(int rows)
{
    if (m_maxRows == rows)
        return;

    m_maxRows = rows;
    invalidateFilter();
    emit filterMaximumRowsChanged(rows);
}

void DataProxyModel::setMinimumInclusiveCostRatio(double minimumInclusiveCost)
{
    if (m_minimumInclusiveCostRatio == minimumInclusiveCost)
        return;

    m_minimumInclusiveCostRatio = minimumInclusiveCost;
    invalidateFilter();
}

void DataProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (!qobject_cast<DataModel *>(sourceModel)) {
        qWarning() << Q_FUNC_INFO << "accepts DataModel instances only";
        return;
    }

    QSortFilterProxyModel::setSourceModel(sourceModel);
}

DataModel *DataProxyModel::dataModel() const
{
    return qobject_cast<DataModel *>(sourceModel());
}

bool DataProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex source_index = sourceModel()->index( source_row, 0, source_parent );
    if (!source_index.isValid())
        return false;

    // if the filter regexp is a non-empty string, ignore our filters
    if (!filterRegularExpression().pattern().isEmpty())
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);

    // check max rows
    if (m_maxRows > 0 && source_row > m_maxRows)
        return false;

    auto func = source_index.data(DataModel::FunctionRole).value<const Function *>();

    if (!func)
        return false;

    // check if func is located in the specific base directory, if any
    if (!m_baseDir.isEmpty()) {
        if (!func->location().startsWith(m_baseDir))
            return false;
    }

    // check if the function from this index is a child of (called by) the filter function
    if (m_function) {
        bool isValid = false;
        const QList<const FunctionCall *> calls = func->incomingCalls();
        for (const FunctionCall *call : calls) {
            if (call->caller() == m_function) {
                isValid = true;
                break;
            }
        }
        if (!isValid)
            return false;
    }

    // check minimum inclusive costs
    DataModel *model = dataModel();
    QTC_ASSERT(model, return false); // as always: this should never happen
    const ParseData *data = model->parseData();
    QTC_ASSERT(data, return false);
    if (m_minimumInclusiveCostRatio != 0.0) {
        const quint64 totalCost = data->totalCost(0);
        const quint64 inclusiveCost = func->inclusiveCost(0);
        const float inclusiveCostRatio = (float)inclusiveCost / totalCost;
        if (inclusiveCostRatio < m_minimumInclusiveCostRatio)
            return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

} // namespace Valgrind::Callgrind
