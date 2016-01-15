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

#include "sourcefileswindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QDebug>

#include <QContextMenuEvent>
#include <QMenu>


//////////////////////////////////////////////////////////////////
//
// SourceFilesWindow
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

SourceFilesTreeView::SourceFilesTreeView()
{
    setSortingEnabled(true);
}

void SourceFilesTreeView::rowActivated(const QModelIndex &index)
{
    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);
    engine->gotoLocation(index.data().toString());
}

void SourceFilesTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);
    QModelIndex index = indexAt(ev->pos());
    index = index.sibling(index.row(), 0);
    QString name = index.data().toString();
    bool engineActionsEnabled = engine->debuggerActionsEnabled();

    QMenu menu;
    QAction *act1 = new QAction(tr("Reload Data"), &menu);

    act1->setEnabled(engineActionsEnabled);
    //act1->setCheckable(true);
    QAction *act2 = 0;
    if (name.isEmpty()) {
        act2 = new QAction(tr("Open File"), &menu);
        act2->setEnabled(false);
    } else {
        act2 = new QAction(tr("Open File \"%1\"'").arg(name), &menu);
        act2->setEnabled(true);
    }

    menu.addAction(act1);
    menu.addAction(act2);
    menu.addSeparator();
    menu.addAction(action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act1)
        engine->reloadSourceFiles();
    else if (act == act2)
        engine->gotoLocation(name);
}

} // namespace Internal
} // namespace Debugger

