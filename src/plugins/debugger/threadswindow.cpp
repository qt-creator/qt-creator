/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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
    : BaseWindow(parent)
{
    setWindowTitle(tr("Thread"));
    setSortingEnabled(true);
    setAlwaysAdjustColumnsAction(debuggerCore()->action(AlwaysAdjustThreadsColumnWidths));
    setObjectName(QLatin1String("ThreadsWindow"));
}

void ThreadsWindow::rowActivated(const QModelIndex &index)
{
    debuggerCore()->currentEngine()->selectThread(index.row());
}

void ThreadsWindow::setModel(QAbstractItemModel *model)
{
    BaseWindow::setModel(model);
    resizeColumnToContents(ThreadData::IdColumn);
    resizeColumnToContents(ThreadData::LineColumn);
    resizeColumnToContents(ThreadData::NameColumn);
    resizeColumnToContents(ThreadData::StateColumn);
    resizeColumnToContents(ThreadData::TargetIdColumn);
}

void ThreadsWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    addBaseContextActions(&menu);
    QAction *act = menu.exec(ev->globalPos());
    handleBaseContextAction(act);
}

} // namespace Internal
} // namespace Debugger
