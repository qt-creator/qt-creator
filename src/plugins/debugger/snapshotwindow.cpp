/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "snapshotwindow.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QTreeView>

static QModelIndexList normalizeIndexes(const QModelIndexList &list)
{
    QModelIndexList res;
    foreach (const QModelIndex &idx, list)
        if (idx.column() == 0)
            res.append(idx);
    return res;
}


namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// SnapshotWindow
//
///////////////////////////////////////////////////////////////////////

SnapshotWindow::SnapshotWindow(QWidget *parent)
    : QTreeView(parent), m_alwaysResizeColumnsToContents(false)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Snapshots"));
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        this, SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

SnapshotWindow::~SnapshotWindow()
{
}

void SnapshotWindow::rowActivated(const QModelIndex &index)
{
    model()->setData(index, index.row(), RequestActivateSnapshotRole);
}

void SnapshotWindow::removeSnapshots(const QModelIndexList &indexes)
{
    QTC_ASSERT(!indexes.isEmpty(), return);
    foreach (const QModelIndex &idx, indexes)
        model()->setData(idx, QVariant(), RequestRemoveSnapshotRole);
}

void SnapshotWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete) {
        QItemSelectionModel *sm = selectionModel();
        QTC_ASSERT(sm, return);
        QModelIndexList si = sm->selectedIndexes();
        if (si.isEmpty())
            si.append(currentIndex().sibling(currentIndex().row(), 0));
        removeSnapshots(normalizeIndexes(si));
    }
    QTreeView::keyPressEvent(ev);
}

void SnapshotWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QModelIndex idx = indexAt(ev->pos());

    QMenu menu;

    QAction *actCreate = menu.addAction(tr("Create Snapshot"));
    actCreate->setEnabled(idx.data(SnapshotCapabilityRole).toBool());

    menu.addSeparator();

    QAction *actRemove = menu.addAction(tr("Remove Snapshot"));
    actRemove->setEnabled(idx.isValid());

    menu.addSeparator();

    QAction *actAdjust = menu.addAction(tr("Adjust Column Widths to Contents"));

    QAction *actAlwaysAdjust =
        menu.addAction(tr("Always Adjust Column Widths to Contents"));
    actAlwaysAdjust->setCheckable(true);
    actAlwaysAdjust->setChecked(m_alwaysResizeColumnsToContents);

    menu.addSeparator();

    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == actCreate)
        model()->setData(idx, idx.row(), RequestMakeSnapshotRole);
    else if (act == actRemove)
        model()->setData(idx, idx.row(), RequestRemoveSnapshotRole);
    else if (act == actAdjust)
        resizeColumnsToContents();
    else if (act == actAlwaysAdjust)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
}

void SnapshotWindow::resizeColumnsToContents()
{
    for (int i = model()->columnCount(); --i >= 0; )
        resizeColumnToContents(i);
}

void SnapshotWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode =
        on ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    for (int i = model()->columnCount(); --i >= 0; )
        header()->setResizeMode(i, mode);
}

} // namespace Internal
} // namespace Debugger
