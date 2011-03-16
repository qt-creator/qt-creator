/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "threadswindow.h"

#include "threadshandler.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>

namespace Debugger {
namespace Internal {

ThreadsWindow::ThreadsWindow(QWidget *parent)
    : QTreeView(parent)
{
    QAction *act = debuggerCore()->action(UseAlternatingRowColors);

    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setWindowTitle(tr("Thread"));
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(debuggerCore()->action(AlwaysAdjustThreadsColumnWidths),
        SIGNAL(toggled(bool)),
        SLOT(setAlwaysResizeColumnsToContents(bool)));
}

void ThreadsWindow::rowActivated(const QModelIndex &index)
{
    debuggerCore()->currentEngine()->selectThread(index.row());
}

void ThreadsWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    resizeColumnToContents(0); // Id
    resizeColumnToContents(4); // Line
    resizeColumnToContents(6); // Name
}

void ThreadsWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *adjustColumnAction =
        menu.addAction(tr("Adjust Column Widths to Contents"));
    menu.addAction(debuggerCore()->action(AlwaysAdjustThreadsColumnWidths));
    menu.addSeparator();

    menu.addAction(debuggerCore()->action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());
    if (!act)
        return;

    if (act == adjustColumnAction)
        resizeColumnsToContents();
}

void ThreadsWindow::resizeColumnsToContents()
{
    const int columnCount = model()->columnCount();
    for (int c = 0 ; c < columnCount; c++)
        resizeColumnToContents(c);
}

void ThreadsWindow::setAlwaysResizeColumnsToContents(bool on)
{
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
}

} // namespace Internal
} // namespace Debugger
