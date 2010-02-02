/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "debuggeragents.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QComboBox>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>


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

SnapshotWindow::SnapshotWindow(DebuggerManager *manager, QWidget *parent)
    : QTreeView(parent), m_manager(manager), m_alwaysResizeColumnsToContents(false)
{
    m_disassemblerAgent = new DisassemblerViewAgent(manager);

    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Snapshots"));

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
    delete m_disassemblerAgent;
}

void SnapshotWindow::rowActivated(const QModelIndex &index)
{
    m_manager->activateSnapshot(index.row());
}

void SnapshotWindow::removeSnapshots(const QModelIndexList &indexes)
{
    QTC_ASSERT(!indexes.isEmpty(), return);
    QList<int> list;
    foreach (const QModelIndex &idx, indexes)
        list.append(idx.row());
    removeSnapshots(list);
}

void SnapshotWindow::removeSnapshots(QList<int> list)
{
    if (list.empty())
        return;
    const int firstRow = list.front();
    qSort(list.begin(), list.end());
    for (int i = list.size(); --i >= 0; )
        m_manager->removeSnapshot(list.at(i));

    const int row = qMin(firstRow, model()->rowCount() - 1);
    if (row >= 0)
        setCurrentIndex(model()->index(row, 0));
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
    //QModelIndex idx = indexAt(ev->pos());

    QMenu menu;

    QAction *actAdjust = menu.addAction(tr("Adjust column widths to contents"));

    QAction *actAlwaysAdjust =
        menu.addAction(tr("Always adjust column widths to contents"));
    actAlwaysAdjust->setCheckable(true);
    actAlwaysAdjust->setChecked(m_alwaysResizeColumnsToContents);

    menu.addSeparator();

    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == actAdjust)
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
