// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stackwindow.h"

#include "stackhandler.h"

#include <QAction>
#include <QHeaderView>

namespace Debugger::Internal {

StackTreeView::StackTreeView(QWidget *parent)
    : BaseTreeView(parent)
{
    setSpanColumn(StackFunctionNameColumn);
}

void StackTreeView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);

    if (model)
        setRootIndex(model->index(0, 0, QModelIndex()));

    connect(static_cast<StackHandler*>(model), &StackHandler::stackChanged, this, [this] {
        if (!m_contentsAdjusted)
            adjustForContents();
    });
}

void StackTreeView::showAddressColumn(bool on)
{
    setColumnHidden(StackAddressColumn, !on);
    adjustForContents(true);
}

void StackTreeView::adjustForContents(bool refreshSpan)
{
    // Skip resizing if no contents. This will be called again once contents are available.
    if (!model() || model()->rowCount() == 0) {
        if (refreshSpan)
            refreshSpanColumn();
        return;
    }

    // Resize without attempting to fix up the columns.
    setSpanColumn(-1);
    resizeColumnToContents(StackLevelColumn);
    resizeColumnToContents(StackFileNameColumn);
    resizeColumnToContents(StackLineNumberColumn);
    resizeColumnToContents(StackAddressColumn);
    setSpanColumn(StackFunctionNameColumn);
    m_contentsAdjusted = true;
}

} // Debugger::Internal
