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

#include "stackwindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "stackhandler.h"

#include <utils/savedaction.h>

#include <QAction>
#include <QHeaderView>

namespace Debugger {
namespace Internal {

StackTreeView::StackTreeView(QWidget *parent)
    : BaseTreeView(parent)
{
    connect(action(UseAddressInStackView), &QAction::toggled,
        this, &StackTreeView::showAddressColumn);
    setSpanColumn(StackFunctionNameColumn);
    showAddressColumn(false);
}

void StackTreeView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);
    connect(static_cast<StackHandler*>(model), &StackHandler::stackChanged,
            this, [this]() {
        if (!m_contentsAdjusted)
            adjustForContents();
    });

    // Resize for the current contents if any are available.
    showAddressColumn(action(UseAddressInStackView)->isChecked());
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

} // namespace Internal
} // namespace Debugger
