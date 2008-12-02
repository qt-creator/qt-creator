/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "stackwindow.h"

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

using Debugger::Internal::StackWindow;

StackWindow::StackWindow(QWidget *parent)
    : QTreeView(parent), m_alwaysResizeColumnsToContents(false)
{
    setWindowTitle(tr("Stack"));

    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        this, SLOT(rowActivated(QModelIndex)));
}

void StackWindow::resizeEvent(QResizeEvent *event)
{
    QHeaderView *hv = header();
    int totalSize = event->size().width() - 120;
    if (totalSize > 10) {
        hv->resizeSection(0, 45);
        hv->resizeSection(1, totalSize / 2);
        hv->resizeSection(2, totalSize / 2);
        hv->resizeSection(3, 55);
    }
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

void StackWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
}

void StackWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
    header()->setResizeMode(2, mode);
}
