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

#include "moduleswindow.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>


///////////////////////////////////////////////////////////////////////////
//
// ModulesWindow
//
///////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

ModulesWindow::ModulesWindow(QWidget *parent)
  : QTreeView(parent), m_alwaysResizeColumnsToContents(false)
{
    QAction *act = debuggerCore()->action(UseAlternatingRowColors);
    setWindowTitle(tr("Modules"));
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setSortingEnabled(true);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(moduleActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
}

void ModulesWindow::moduleActivated(const QModelIndex &index)
{
    debuggerCore()->gotoLocation(index.data().toString());
}

void ModulesWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QString name;
    QModelIndex index = indexAt(ev->pos());
    if (index.isValid())
        index = index.sibling(index.row(), 0);
    if (index.isValid())
        name = index.data().toString();

    DebuggerEngine *engine = debuggerCore()->currentEngine();
    const bool enabled = engine->debuggerActionsEnabled();
    const unsigned capabilities = engine->debuggerCapabilities();

    QMenu menu;

    QAction *actUpdateModuleList
        = new QAction(tr("Update Module List"), &menu);
    actUpdateModuleList
        ->setEnabled(enabled && (capabilities & ReloadModuleCapability));

    QAction *actShowModuleSources
        = new QAction(tr("Show Source Files for Module \"%1\"").arg(name), &menu);
    actShowModuleSources
        ->setEnabled(enabled && (capabilities & ReloadModuleCapability));

    QAction *actLoadSymbolsForAllModules
        = new QAction(tr("Load Symbols for All Modules"), &menu);
    actLoadSymbolsForAllModules
        -> setEnabled(enabled && (capabilities & ReloadModuleSymbolsCapability));

    QAction *actExamineAllModules
        = new QAction(tr("Examine All Modules"), &menu);
    actExamineAllModules
        -> setEnabled(enabled && (capabilities & ReloadModuleSymbolsCapability));

    QAction *actLoadSymbolsForModule = 0;
    QAction *actEditFile = 0;
    QAction *actShowModuleSymbols = 0;
    if (name.isEmpty()) {
        actLoadSymbolsForModule = new QAction(tr("Load Symbols for Module"), &menu);
        actLoadSymbolsForModule->setEnabled(false);
        actEditFile = new QAction(tr("Edit File"), &menu);
        actEditFile->setEnabled(false);
        actShowModuleSymbols = new QAction(tr("Show Symbols"), &menu);
        actShowModuleSymbols->setEnabled(false);
    } else {
        actLoadSymbolsForModule
            = new QAction(tr("Load Symbols for Module \"%1\"").arg(name), &menu);
        actLoadSymbolsForModule
            ->setEnabled(capabilities & ReloadModuleSymbolsCapability);
        actEditFile
            = new QAction(tr("Edit File \"%1\"").arg(name), &menu);
        actShowModuleSymbols
            = new QAction(tr("Show Symbols in File \"%1\"").arg(name), &menu);
        actShowModuleSymbols
            ->setEnabled(capabilities & ShowModuleSymbolsCapability);
    }

    menu.addAction(actUpdateModuleList);
    //menu.addAction(actShowModuleSources);  // FIXME
    menu.addAction(actLoadSymbolsForAllModules);
    menu.addAction(actExamineAllModules);
    menu.addAction(actLoadSymbolsForModule);
    menu.addAction(actEditFile);
    menu.addAction(actShowModuleSymbols);
    menu.addSeparator();
    QAction *actAdjustColumnWidths =
        menu.addAction(tr("Adjust Column Widths to Contents"));
    QAction *actAlwaysAdjustColumnWidth =
        menu.addAction(tr("Always Adjust Column Widths to Contents"));
    actAlwaysAdjustColumnWidth->setCheckable(true);
    actAlwaysAdjustColumnWidth->setChecked(m_alwaysResizeColumnsToContents);
    menu.addSeparator();
    menu.addAction(debuggerCore()->action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == actUpdateModuleList)
        engine->reloadModules();
    else if (act == actAdjustColumnWidths)
      resizeColumnsToContents();
    else if (act == actAlwaysAdjustColumnWidth)
      setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == actShowModuleSources)
      engine->loadSymbols(name);
    else if (act == actLoadSymbolsForAllModules)
      engine->loadAllSymbols();
    else if (act == actExamineAllModules)
      engine->examineModules();
    else if (act == actLoadSymbolsForModule)
      engine->loadSymbols(name);
    else if (act == actEditFile)
      debuggerCore()->gotoLocation(name);
    else if (act == actShowModuleSymbols)
      engine->requestModuleSymbols(name);
}

void ModulesWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

void ModulesWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
    header()->setResizeMode(2, mode);
    header()->setResizeMode(3, mode);
    header()->setResizeMode(4, mode);
    //setColumnHidden(3, true);
}

void ModulesWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setAlwaysResizeColumnsToContents(true);
}

} // namespace Internal
} // namespace Debugger
