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
#include "stackhandler.h"

#include <utils/qtcassert.h>

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QResizeEvent>
#include <QTreeView>
#include <QVBoxLayout>

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

void ThreadsWindow::resizeEvent(QResizeEvent *event)
{
    //QHeaderView *hv = header();
    //int totalSize = event->size().width() - 120;
    //hv->resizeSection(0, 45);
    //hv->resizeSection(1, totalSize);
    //hv->resizeSection(2, 55);
    QTreeView::resizeEvent(event);
}

void ThreadsWindow::rowActivated(const QModelIndex &index)
{
    //qDebug() << "ACTIVATED: " << index.row() << index.column();
    emit threadSelected(index.row());
}

void ThreadsWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *act1 = menu.addAction(tr("Adjust column widths to contents"));
    QAction *act2 = menu.addAction(tr("Always adjust column widths to contents"));
    act2->setCheckable(true);
    act2->setChecked(m_alwaysResizeColumnsToContents);

    menu.addSeparator();

    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act1)
        resizeColumnsToContents();
    else if (act == act2)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
}

void ThreadsWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    //resizeColumnToContents(1);
}

void ThreadsWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    //header()->setResizeMode(1, mode);
}

