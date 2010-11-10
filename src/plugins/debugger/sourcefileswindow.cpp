/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "sourcefileswindow.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>


//////////////////////////////////////////////////////////////////
//
// SourceFilesWindow
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

static DebuggerEngine *currentEngine()
{
    return debuggerCore()->currentEngine();
}

SourceFilesWindow::SourceFilesWindow(QWidget *parent)
    : QTreeView(parent)
{
    QAction *act = debuggerCore()->action(UseAlternatingRowColors);

    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setWindowTitle(tr("Source Files"));
    setSortingEnabled(true);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));
    //header()->setDefaultAlignment(Qt::AlignLeft);

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(sourceFileActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
}

void SourceFilesWindow::sourceFileActivated(const QModelIndex &index)
{
    debuggerCore()->gotoLocation(index.data().toString());
}

void SourceFilesWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QModelIndex index = indexAt(ev->pos());
    index = index.sibling(index.row(), 0);
    QString name = index.data().toString();
    bool engineActionsEnabled = currentEngine()->debuggerActionsEnabled();

    QMenu menu;
    QAction *act1 = new QAction(tr("Reload Data"), &menu);

    act1->setEnabled(engineActionsEnabled);
    //act1->setCheckable(true);
    QAction *act2 = 0;
    if (name.isEmpty()) {
        act2 = new QAction(tr("Open File"), &menu);
        act2->setEnabled(false);
    } else {
        act2 = new QAction(tr("Open File \"%1\"'").arg(name), &menu);
        act2->setEnabled(true);
    }

    menu.addAction(act1);
    menu.addAction(act2);
    menu.addSeparator();
    menu.addAction(debuggerCore()->action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act1)
        currentEngine()->reloadSourceFiles();
    else if (act == act2)
        debuggerCore()->gotoLocation(name);
}

} // namespace Internal
} // namespace Debugger

