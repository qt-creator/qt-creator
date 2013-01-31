/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "snapshotwindow.h"
#include "snapshothandler.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QDebug>

#include <QHeaderView>
#include <QMenu>
#include <QKeyEvent>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// SnapshotWindow
//
///////////////////////////////////////////////////////////////////////

SnapshotTreeView::SnapshotTreeView(SnapshotHandler *handler)
{
    m_snapshotHandler = handler;
    setWindowTitle(tr("Snapshots"));
    setAlwaysAdjustColumnsAction(debuggerCore()->action(AlwaysAdjustSnapshotsColumnWidths));
}

void SnapshotTreeView::rowActivated(const QModelIndex &index)
{
    m_snapshotHandler->activateSnapshot(index.row());
}

void SnapshotTreeView::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete) {
        QItemSelectionModel *sm = selectionModel();
        QTC_ASSERT(sm, return);
        QModelIndexList si = sm->selectedIndexes();
        if (si.isEmpty())
            si.append(currentIndex().sibling(currentIndex().row(), 0));

        foreach (const QModelIndex &idx, si)
            if (idx.column() == 0)
                removeSnapshot(idx.row());
    }
    QTreeView::keyPressEvent(ev);
}

void SnapshotTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    QModelIndex idx = indexAt(ev->pos());

    QMenu menu;

    QAction *actCreate = menu.addAction(tr("Create Snapshot"));
    actCreate->setEnabled(idx.data(SnapshotCapabilityRole).toBool());
    menu.addSeparator();

    QAction *actRemove = menu.addAction(tr("Remove Snapshot"));
    actRemove->setEnabled(idx.isValid());

    addBaseContextActions(&menu);

    QAction *act = menu.exec(ev->globalPos());

    if (act == actCreate)
        m_snapshotHandler->createSnapshot(idx.row());
    else if (act == actRemove)
        removeSnapshot(idx.row());
    else
        handleBaseContextAction(act);
}

void SnapshotTreeView::removeSnapshot(int i)
{
    m_snapshotHandler->at(i)->quitDebugger();
}

} // namespace Internal
} // namespace Debugger
