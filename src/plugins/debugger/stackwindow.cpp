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

namespace Debugger {
namespace Internal {

StackTreeView::StackTreeView()
{
    setWindowTitle(tr("Stack"));

    connect(action(UseAddressInStackView), &QAction::toggled,
        this, &StackTreeView::showAddressColumn);
    showAddressColumn(false);
}

void StackTreeView::showAddressColumn(bool on)
{
    setColumnHidden(StackAddressColumn, !on);
    resizeColumnToContents(StackLevelColumn);
    resizeColumnToContents(StackLineNumberColumn);
    resizeColumnToContents(StackAddressColumn);
}

void StackTreeView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);
    resizeColumnToContents(StackLevelColumn);
    resizeColumnToContents(StackLineNumberColumn);
    showAddressColumn(action(UseAddressInStackView)->isChecked());
}

} // namespace Internal
} // namespace Debugger
