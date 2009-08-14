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

#include "disassemblerwindow.h"
#include "debuggeractions.h"

#include <QAction>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QResizeEvent>


using namespace Debugger::Internal;

DisassemblerWindow::DisassemblerWindow()
    : m_alwaysResizeColumnsToContents(true), m_alwaysReloadContents(false)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Disassembler"));
    setSortingEnabled(false);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    header()->hide();
    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

void DisassemblerWindow::resizeEvent(QResizeEvent *ev)
{
    //QHeaderView *hv = header();
    //int totalSize = ev->size().width() - 110;
    //hv->resizeSection(0, 60);
    //hv->resizeSection(1, (totalSize * 50) / 100);
    //hv->resizeSection(2, (totalSize * 50) / 100);
    //hv->resizeSection(3, 50);
    QTreeView::resizeEvent(ev);
}

void DisassemblerWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;

    QAction *act1 = new QAction(tr("Adjust column widths to contents"), &menu);
    QAction *act2 = new QAction(tr("Always adjust column widths to contents"), &menu);
    act2->setCheckable(true);
    // FIXME: make this a SavedAction
    act2->setChecked(m_alwaysResizeColumnsToContents);
    QAction *act3 = new QAction(tr("Reload disassembler listing"), &menu);
    QAction *act4 = new QAction(tr("Always reload disassembler listing"), &menu);
    act4->setCheckable(true);
    act4->setChecked(m_alwaysReloadContents);
    menu.addAction(act3);
    //menu.addAction(act4);
    menu.addSeparator();
    menu.addAction(act1);
    menu.addAction(act2);
    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act1)
        resizeColumnsToContents();
    else if (act == act2)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == act3)
        reloadContents();
    else if (act == act2)
        setAlwaysReloadContents(!m_alwaysReloadContents);
}

void DisassemblerWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
}

void DisassemblerWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
    header()->setResizeMode(2, mode);
}

void DisassemblerWindow::setAlwaysReloadContents(bool on)
{
    m_alwaysReloadContents = on;
    if (m_alwaysReloadContents)
        reloadContents();
}

void DisassemblerWindow::reloadContents()
{
    emit reloadDisassemblerRequested();
}


void DisassemblerWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setAlwaysResizeColumnsToContents(true);
}

