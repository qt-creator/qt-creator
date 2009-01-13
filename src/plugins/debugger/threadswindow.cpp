/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "threadswindow.h"

#include "assert.h"
#include "stackhandler.h"

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
    setWindowTitle(tr("Thread"));

    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(const QModelIndex &)),
        this, SLOT(rowActivated(const QModelIndex &)));
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
    QAction *act1 = new QAction(tr("Adjust column widths to contents"), &menu);
    QAction *act2 = new QAction(tr("Always adjust column widths to contents"), &menu);
    act2->setCheckable(true);
    act2->setChecked(m_alwaysResizeColumnsToContents);
    menu.addAction(act1);
    menu.addAction(act2);

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

