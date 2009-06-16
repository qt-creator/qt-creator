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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "stackwindow.h"

#include "debuggeractions.h"

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


using Debugger::Internal::StackWindow;

StackWindow::StackWindow(QWidget *parent)
    : QTreeView(parent), m_alwaysResizeColumnsToContents(false)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Stack"));

    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        this, SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

void StackWindow::resizeEvent(QResizeEvent *event)
{
/*
    QHeaderView *hv = header();

    int totalSize = event->size().width() - 120;
    if (totalSize > 10) {
        hv->resizeSection(0, 45);
        hv->resizeSection(1, totalSize / 2);
        hv->resizeSection(2, totalSize / 2);
        hv->resizeSection(3, 55);
    }
*/
    QTreeView::resizeEvent(event);
}

void StackWindow::rowActivated(const QModelIndex &index)
{
    //qDebug() << "ACTIVATED: " << index.row() << index.column();
    emit frameActivated(index.row());
}

void StackWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;

    QAction *act0 = new QAction(tr("Copy contents to clipboard"), &menu);
    act0->setEnabled(model() != 0);

    QAction *act1 = new QAction(tr("Adjust column widths to contents"), &menu);

    QAction *act2 = new QAction(tr("Always adjust column widths to contents"), &menu);
    act2->setCheckable(true);
    act2->setChecked(m_alwaysResizeColumnsToContents);

    menu.addAction(theDebuggerAction(ExpandStack));
    menu.addAction(act0);
    menu.addSeparator();
    menu.addAction(act1);
    menu.addAction(act2);
    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act0)
        copyContentsToClipboard();
    else if (act == act1)
        resizeColumnsToContents();
    else if (act == act2)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
}

void StackWindow::copyContentsToClipboard()
{
    QString str;
    int n = model()->rowCount();
    int m = model()->columnCount();
    for (int i = 0; i != n; ++i) {
        for (int j = 0; j != m; ++j) {
            QModelIndex index = model()->index(i, j);
            str += model()->data(index).toString();
            str += '\t';
        }
        str += '\n';
    }
    QClipboard *clipboard = QApplication::clipboard();
    #ifdef Q_WS_X11
    clipboard->setText(str, QClipboard::Selection);
    #endif
    clipboard->setText(str, QClipboard::Clipboard);
}

void StackWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

void StackWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode =
        on ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
    header()->setResizeMode(2, mode);
    header()->setResizeMode(3, mode);
}
