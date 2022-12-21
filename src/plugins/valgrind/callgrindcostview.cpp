// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindcostview.h"

#include "callgrindnamedelegate.h"

#include <valgrind/callgrind/callgrindabstractmodel.h>
#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindcallmodel.h>

#include <QAbstractProxyModel>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDebug>

using namespace Valgrind::Callgrind;

namespace Valgrind {
namespace Internal {

CostView::CostView(QWidget *parent)
    : Utils::BaseTreeView(parent)
    , m_costDelegate(new CostDelegate(this))
    , m_nameDelegate(new NameDelegate(this))
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setUniformRowHeights(true);
    setAutoScroll(false);
    setSortingEnabled(true);
    setRootIsDecorated(false);
}

CostView::~CostView() = default;

void CostView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);

    forever {
        auto proxy = qobject_cast<const QAbstractProxyModel *>(model);
        if (!proxy)
            break;
        model = proxy->sourceModel();
    }

    setItemDelegate(new QStyledItemDelegate(this));

    if (qobject_cast<CallModel *>(model)) {
        setItemDelegateForColumn(CallModel::CalleeColumn, m_nameDelegate);
        setItemDelegateForColumn(CallModel::CallerColumn, m_nameDelegate);
        setItemDelegateForColumn(CallModel::CostColumn, m_costDelegate);
    } else if (qobject_cast<DataModel *>(model)) {
        setItemDelegateForColumn(DataModel::InclusiveCostColumn, m_costDelegate);
        setItemDelegateForColumn(DataModel::NameColumn, m_nameDelegate);
        setItemDelegateForColumn(DataModel::SelfCostColumn, m_costDelegate);
    }

    m_costDelegate->setModel(model);
}

void CostView::setCostFormat(CostDelegate::CostFormat format)
{
    m_costDelegate->setFormat(format);
    viewport()->update();
}

CostDelegate::CostFormat CostView::costFormat() const
{
    return m_costDelegate->format();
}

} // namespace Internal
} // namespace Valgrind
