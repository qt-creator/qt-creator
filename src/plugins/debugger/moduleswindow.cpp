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

#include "moduleswindow.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>

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
    DebuggerEngine *engine = debuggerCore()->currentEngine();
    QTC_ASSERT(engine, return);
    engine->gotoLocation(index.data().toString());
}

void ModulesWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QString name;
    QString fileName;
    QModelIndex index = indexAt(ev->pos());
    if (index.isValid())
        index = index.sibling(index.row(), 0);
    if (index.isValid()) {
        name = index.data().toString();
        fileName = index.sibling(index.row(), 1).data().toString();
    }

    DebuggerEngine *engine = debuggerCore()->currentEngine();
    QTC_ASSERT(engine, return);
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
    QAction *actShowDependencies = 0; // Show dependencies by running 'depends.exe'
    if (name.isEmpty()) {
        actLoadSymbolsForModule = new QAction(tr("Load Symbols for Module"), &menu);
        actLoadSymbolsForModule->setEnabled(false);
        actEditFile = new QAction(tr("Edit File"), &menu);
        actEditFile->setEnabled(false);
        actShowModuleSymbols = new QAction(tr("Show Symbols"), &menu);
        actShowModuleSymbols->setEnabled(false);
#ifdef Q_OS_WIN
        actShowDependencies = new QAction(tr("Show Dependencies"), &menu);
        actShowDependencies->setEnabled(false);
#endif
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
#ifdef Q_OS_WIN
        actShowDependencies = new QAction(tr("Show Dependencies of \"%1\"").arg(name), &menu);
        actShowDependencies->setEnabled(!fileName.isEmpty());
#endif
    }

    menu.addAction(actUpdateModuleList);
    //menu.addAction(actShowModuleSources);  // FIXME
    if (actShowDependencies)
        menu.addAction(actShowDependencies);
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
      engine->gotoLocation(name);
    else if (act == actShowModuleSymbols)
      engine->requestModuleSymbols(name);
    else if (actShowDependencies && act == actShowDependencies)
        QProcess::startDetached(QLatin1String("depends"), QStringList(fileName));
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
