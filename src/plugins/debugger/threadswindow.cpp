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

#include "threadswindow.h"

#include "debuggeractions.h"

#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>

using Debugger::Internal::ThreadsWindow;

ThreadsWindow::ThreadsWindow(QWidget *parent)
    : QTreeView(parent), m_alwaysResizeColumnsToContents(false)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);

    setWindowTitle(tr("Thread"));
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        this, SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

void ThreadsWindow::rowActivated(const QModelIndex &index)
{
    emit threadSelected(index.row());
}

void ThreadsWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *adjustColumnAction =
        menu.addAction(tr("Adjust Column Widths to Contents"));
    QAction *alwaysAdjustColumnAction =
        menu.addAction(tr("Always Adjust Column Widths to Contents"));
    alwaysAdjustColumnAction->setCheckable(true);
    alwaysAdjustColumnAction->setChecked(m_alwaysResizeColumnsToContents);
    menu.addSeparator();

    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());
    if(!act)
        return;

    if (act == adjustColumnAction) {
        resizeColumnsToContents();
    } else if (act == alwaysAdjustColumnAction) {
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    }
}

void ThreadsWindow::resizeColumnsToContents()
{
    const int columnCount = model()->columnCount();
    for (int c = 0 ; c < columnCount; c++)
        resizeColumnToContents(c);
}

void ThreadsWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
}
