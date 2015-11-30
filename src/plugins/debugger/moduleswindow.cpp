/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "moduleswindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QDebug>
#include <QMenu>
#include <QProcess>
#include <QResizeEvent>


///////////////////////////////////////////////////////////////////////////
//
// ModulesWindow
//
///////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

ModulesTreeView::ModulesTreeView()
{
    setSortingEnabled(true);

    connect(this, &QAbstractItemView::activated,
            this, &ModulesTreeView::moduleActivated);
}

void ModulesTreeView::moduleActivated(const QModelIndex &index)
{
    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);
    if (index.isValid())
        engine->gotoLocation(index.sibling(index.row(), 1).data().toString());
}

void ModulesTreeView::contextMenuEvent(QContextMenuEvent *ev)
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

    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);
    const bool enabled = engine->debuggerActionsEnabled();
    const bool canReload = engine->hasCapability(ReloadModuleCapability);
    const bool canLoadSymbols = engine->hasCapability(ReloadModuleSymbolsCapability);

    QMenu menu;

    QAction *actUpdateModuleList
        = new QAction(tr("Update Module List"), &menu);
    actUpdateModuleList->setEnabled(enabled && canReload);

    QAction *actShowModuleSources
        = new QAction(tr("Show Source Files for Module \"%1\"").arg(name), &menu);
    actShowModuleSources->setEnabled(enabled && canReload);

    QAction *actLoadSymbolsForAllModules
        = new QAction(tr("Load Symbols for All Modules"), &menu);
    actLoadSymbolsForAllModules->setEnabled(enabled && canLoadSymbols);

    QAction *actExamineAllModules
        = new QAction(tr("Examine All Modules"), &menu);
    actExamineAllModules->setEnabled(enabled && canLoadSymbols);

    QAction *actLoadSymbolsForModule = 0;
    QAction *actEditFile = 0;
    QAction *actShowModuleSymbols = 0;
    QAction *actShowModuleSections = 0;
    QAction *actShowDependencies = 0; // Show dependencies by running 'depends.exe'
    if (name.isEmpty()) {
        actLoadSymbolsForModule = new QAction(tr("Load Symbols for Module"), &menu);
        actLoadSymbolsForModule->setEnabled(false);
        actEditFile = new QAction(tr("Edit File"), &menu);
        actEditFile->setEnabled(false);
        actShowModuleSymbols = new QAction(tr("Show Symbols"), &menu);
        actShowModuleSymbols->setEnabled(false);
        actShowModuleSections = new QAction(tr("Show Sections"), &menu);
        actShowModuleSections->setEnabled(false);
        actShowDependencies = new QAction(tr("Show Dependencies"), &menu);
        actShowDependencies->setEnabled(false);
    } else {
        actLoadSymbolsForModule
            = new QAction(tr("Load Symbols for Module \"%1\"").arg(name), &menu);
        actLoadSymbolsForModule->setEnabled(canLoadSymbols);
        actEditFile
            = new QAction(tr("Edit File \"%1\"").arg(name), &menu);
        actShowModuleSymbols
            = new QAction(tr("Show Symbols in File \"%1\"").arg(name), &menu);
        actShowModuleSymbols->setEnabled(engine->hasCapability(ShowModuleSymbolsCapability));
        actShowModuleSections
            = new QAction(tr("Show Sections in File \"%1\"").arg(name), &menu);
        actShowModuleSections->setEnabled(engine->hasCapability(ShowModuleSymbolsCapability));
        actShowDependencies = new QAction(tr("Show Dependencies of \"%1\"").arg(name), &menu);
        actShowDependencies->setEnabled(!fileName.isEmpty());
        if (!Utils::HostOsInfo::isWindowsHost())
            // FIXME: Dependencies only available on Windows, when "depends" is installed.
            actShowDependencies->setEnabled(false);
    }

    menu.addAction(actUpdateModuleList);
    //menu.addAction(actShowModuleSources);  // FIXME
    menu.addAction(actShowDependencies);
    menu.addAction(actLoadSymbolsForAllModules);
    menu.addAction(actExamineAllModules);
    menu.addAction(actLoadSymbolsForModule);
    menu.addAction(actEditFile);
    menu.addAction(actShowModuleSymbols);
    menu.addAction(actShowModuleSections);
    menu.addSeparator();
    menu.addAction(action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == actUpdateModuleList)
        engine->reloadModules();
    else if (act == actShowModuleSources)
        engine->loadSymbols(fileName);
    else if (act == actLoadSymbolsForAllModules)
        engine->loadAllSymbols();
    else if (act == actExamineAllModules)
        engine->examineModules();
    else if (act == actLoadSymbolsForModule)
        engine->loadSymbols(fileName);
    else if (act == actEditFile)
        engine->gotoLocation(fileName);
    else if (act == actShowModuleSymbols)
        engine->requestModuleSymbols(fileName);
    else if (act == actShowModuleSections)
        engine->requestModuleSections(fileName);
    else if (actShowDependencies && act == actShowDependencies)
        QProcess::startDetached(QLatin1String("depends"), QStringList(fileName));
}

} // namespace Internal
} // namespace Debugger
