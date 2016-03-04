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

#include "callgrindcallmodel.h"

#include "callgrindfunctioncall.h"
#include "callgrindfunction.h"
#include "callgrindparsedata.h"

#include <utils/qtcassert.h>

#include <QVector>

namespace Valgrind {
namespace Callgrind {

class CallModel::Private
{
public:
    const ParseData *m_data = 0;
    QVector<const FunctionCall *> m_calls;
    int m_event = 0;
    const Function *m_function = 0;
};

CallModel::CallModel()
  : d(new Private)
{
}

CallModel::~CallModel()
{
    delete d;
}

void CallModel::clear()
{
    beginResetModel();
    d->m_function = 0;
    d->m_calls.clear();
    endResetModel();
}

void CallModel::setCalls(const QVector<const FunctionCall *> &calls, const Function *function)
{
    beginResetModel();
    d->m_function = function;
    d->m_calls = calls;
    endResetModel();
}

QVector<const FunctionCall *> CallModel::calls() const
{
    return d->m_calls;
}

const Function *CallModel::function() const
{
    return d->m_function;
}

void CallModel::setCostEvent(int event)
{
    d->m_event = event;
}

int CallModel::costEvent() const
{
    return d->m_event;
}

void CallModel::setParseData(const ParseData *data)
{
    if (d->m_data == data)
        return;

    if (!data)
        clear();

    d->m_data = data;
}

const ParseData *CallModel::parseData() const
{
    return d->m_data;
}

int CallModel::rowCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);

    if (parent.isValid())
        return 0;

    return d->m_calls.count();
}

int CallModel::columnCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);

    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QModelIndex CallModel::parent(const QModelIndex &child) const
{
    QTC_ASSERT(!child.isValid() || child.model() == this, return QModelIndex());

    return QModelIndex();
}

QModelIndex CallModel::index(int row, int column, const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return QModelIndex());

    if (row == 0 && rowCount(parent) == 0) // happens with empty models
        return QModelIndex();

    QTC_ASSERT(row >= 0 && row < rowCount(parent), return QModelIndex());

    return createIndex(row, column);
}

QVariant CallModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const FunctionCall *call = d->m_calls.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        if (index.column() == CalleeColumn)
            return call->callee()->name();
        if (index.column() == CallerColumn)
            return call->caller()->name();
        if (index.column() == CostColumn && role != Qt::ToolTipRole)
            return call->cost(d->m_event);
        if (index.column() == CallsColumn && role != Qt::ToolTipRole)
            return call->calls();
        return QVariant();
    }

    if (role == ParentCostRole) {
        const quint64 parentCost = d->m_function->inclusiveCost(d->m_event);
        return parentCost;
    }

    if (role == FunctionCallRole)
        return QVariant::fromValue(call);

    if (role == RelativeTotalCostRole) {
        const quint64 totalCost = d->m_data->totalCost(d->m_event);
        const quint64 callCost = call->cost(d->m_event);
        return totalCost ? (double(callCost) / totalCost) : 0.0;
    }

    if (role == RelativeParentCostRole) {
        const quint64 parentCost = d->m_function->inclusiveCost(d->m_event);
        const quint64 callCost = call->cost(d->m_event);
        return parentCost ? (double(callCost) / parentCost) : 0.0;
    }

    return QVariant();
}

QVariant CallModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
        return QVariant();

    QTC_ASSERT(section >= 0 && section < columnCount(), return QVariant());

    if (role == Qt::ToolTipRole) {
        if (section == CostColumn) {
            if (!d->m_data)
                return QVariant();

            return ParseData::prettyStringForEvent(d->m_data->events().at(d->m_event));
        }
        return QVariant();
    }

    if (section == CalleeColumn)
        return tr("Callee");
    else if (section == CallerColumn)
        return tr("Caller");
    else if (section == CostColumn)
        return tr("Cost");
    else if (section == CallsColumn)
        return tr("Calls");

    return QVariant();
}

} // namespace Callgrind
} // namespace Valgrind
