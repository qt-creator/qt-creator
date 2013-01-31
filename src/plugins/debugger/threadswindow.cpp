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

#include "threadswindow.h"

#include "threadshandler.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/savedaction.h>

#include <QDebug>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

namespace Debugger {
namespace Internal {

ThreadsTreeView::ThreadsTreeView()
{
    setSortingEnabled(true);
    setAlwaysAdjustColumnsAction(debuggerCore()->action(AlwaysAdjustThreadsColumnWidths));
}

void ThreadsTreeView::rowActivated(const QModelIndex &index)
{
    ThreadId id = ThreadId(index.data(ThreadData::IdRole).toLongLong());
    debuggerCore()->currentEngine()->selectThread(id);
}

void ThreadsTreeView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);
    resizeColumnToContents(ThreadData::IdColumn);
    resizeColumnToContents(ThreadData::LineColumn);
    resizeColumnToContents(ThreadData::NameColumn);
    resizeColumnToContents(ThreadData::StateColumn);
    resizeColumnToContents(ThreadData::TargetIdColumn);
    resizeColumnToContents(ThreadData::DetailsColumn);
}

void ThreadsTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    addBaseContextActions(&menu);
    QAction *act = menu.exec(ev->globalPos());
    handleBaseContextAction(act);
}

ThreadsWindow::ThreadsWindow()
    : BaseWindow(new ThreadsTreeView)
{
    setWindowTitle(tr("Threads"));
    setObjectName(QLatin1String("ThreadsWindow"));
}

} // namespace Internal
} // namespace Debugger
