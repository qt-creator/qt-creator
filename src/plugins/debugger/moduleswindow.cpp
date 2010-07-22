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

#include "debuggerconstants.h"
#include "debuggeractions.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QAction>
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
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Modules"));
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setSortingEnabled(true);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    connect(this, SIGNAL(activated(QModelIndex)),
        this, SLOT(moduleActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

void ModulesWindow::moduleActivated(const QModelIndex &index)
{
    qDebug() << "ACTIVATED: " << index.row() << index.column()
        << index.data().toString();
    setModelData(RequestOpenFileRole, index.data().toString());
}

void ModulesWindow::resizeEvent(QResizeEvent *event)
{
    //QHeaderView *hv = header();
    //int totalSize = event->size().width() - 110;
    //hv->resizeSection(0, totalSize / 4);
    //hv->resizeSection(1, totalSize / 4);
    //hv->resizeSection(2, totalSize / 4);
    //hv->resizeSection(3, totalSize / 4);
    //hv->resizeSection(0, 60);
    //hv->resizeSection(1, (totalSize * 50) / 100);
    //hv->resizeSection(2, (totalSize * 50) / 100);
    //hv->resizeSection(3, 50);
    //setColumnHidden(3, true);
    QTreeView::resizeEvent(event);
}

void ModulesWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QString name;
    QModelIndex index = indexAt(ev->pos());
    if (index.isValid())
        index = index.sibling(index.row(), 0);
    if (index.isValid())
        name = index.data().toString();

    const bool enabled =
        model() && model()->data(index, EngineActionsEnabledRole).toBool();
    const unsigned capabilities =
        model()->data(index, EngineCapabilitiesRole).toInt();

    QMenu menu;

    QAction *actUpdateModuleList
        = new QAction(tr("Update Module List"), &menu);
    actUpdateModuleList
        ->setEnabled(enabled && (capabilities & ReloadModuleCapability));

    QAction *actShowSourceFiles
        = new QAction(tr("Show Source Files for Module \"%1\"").arg(name), &menu);
    actShowSourceFiles
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
    QAction *actShowSymbols = 0;
    if (name.isEmpty()) {
        actLoadSymbolsForModule = new QAction(tr("Load Symbols for Module"), &menu);
        actLoadSymbolsForModule->setEnabled(false);
        actEditFile = new QAction(tr("Edit File"), &menu);
        actEditFile->setEnabled(false);
        actShowSymbols = new QAction(tr("Show Symbols"), &menu);
        actShowSymbols->setEnabled(false);
    } else {
        actLoadSymbolsForModule
            = new QAction(tr("Load Symbols for Module \"%1\"").arg(name), &menu);
        actLoadSymbolsForModule
            ->setEnabled(capabilities & ReloadModuleSymbolsCapability);
        actEditFile
            = new QAction(tr("Edit File \"%1\"").arg(name), &menu);
        actShowSymbols
            = new QAction(tr("Show Symbols in File \"%1\"").arg(name), &menu);
    }

    menu.addAction(actUpdateModuleList);
    //menu.addAction(actShowSourceFiles); // FIXME
    menu.addAction(actLoadSymbolsForAllModules);
    menu.addAction(actExamineAllModules);
    menu.addAction(actLoadSymbolsForModule);
    menu.addAction(actEditFile);
    //menu.addAction(actShowSymbols); // FIXME
    menu.addSeparator();
    QAction *actAdjustColumnWidths =
        menu.addAction(tr("Adjust Column Widths to Contents"));
    QAction *actAlwaysAdjustColumnWidth =
        menu.addAction(tr("Always Adjust Column Widths to Contents"));
    actAlwaysAdjustColumnWidth->setCheckable(true);
    actAlwaysAdjustColumnWidth->setChecked(m_alwaysResizeColumnsToContents);
    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == actUpdateModuleList) {
        setModelData(RequestReloadModulesRole);
    } else if (act == actAdjustColumnWidths) {
        resizeColumnsToContents();
    } else if (act == actAlwaysAdjustColumnWidth) {
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    //} else if (act == actShowSourceFiles) {
    //    emit displaySourceRequested(name);
    } else if (act == actLoadSymbolsForAllModules) {
        setModelData(RequestAllSymbolsRole);
    } else if (act == actExamineAllModules) {
        setModelData(RequestExamineModulesRole);
    } else if (act == actLoadSymbolsForModule) {
        setModelData(RequestModuleSymbolsRole, name);
    } else if (act == actEditFile) {
        setModelData(RequestOpenFileRole, name);
    } else if (act == actShowSymbols) {
        setModelData(RequestModuleSymbolsRole, name);
    }
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

void ModulesWindow::setModelData
    (int role, const QVariant &value, const QModelIndex &index)
{
    QTC_ASSERT(model(), return);
    model()->setData(index, value, role);
}

} // namespace Internal
} // namespace Debugger
