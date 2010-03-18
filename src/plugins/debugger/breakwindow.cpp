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

#include "breakwindow.h"

#include "debuggeractions.h"
#include "debuggermanager.h"
#include "ui_breakcondition.h"
#include "ui_breakbyfunction.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFileInfoList>

#include <QtGui/QAction>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QToolButton>
#include <QtGui/QTreeView>

using Debugger::Internal::BreakWindow;


///////////////////////////////////////////////////////////////////////
//
// BreakByFunctionDialog
//
///////////////////////////////////////////////////////////////////////

class BreakByFunctionDialog : public QDialog, Ui::BreakByFunctionDialog
{
public:
    explicit BreakByFunctionDialog(QWidget *parent)
      : QDialog(parent)
    {
        setupUi(this);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    }
    QString functionName() const { return functionLineEdit->text(); }
};


///////////////////////////////////////////////////////////////////////
//
// BreakWindow
//
///////////////////////////////////////////////////////////////////////

BreakWindow::BreakWindow(Debugger::DebuggerManager *manager)
  : m_manager(manager), m_alwaysResizeColumnsToContents(false)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setFrameStyle(QFrame::NoFrame);
    setWindowTitle(tr("Breakpoints"));
    setWindowIcon(QIcon(":/debugger/images/debugger_breakpoints.png"));
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(this, SIGNAL(activated(QModelIndex)),
        this, SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
    connect(theDebuggerAction(UseAddressInBreakpointsView), SIGNAL(toggled(bool)),
        this, SLOT(showAddressColumn(bool)));
}

void BreakWindow::showAddressColumn(bool on)
{
    setColumnHidden(6, !on);
}

static QModelIndexList normalizeIndexes(const QModelIndexList &list)
{
    QModelIndexList res;
    foreach (const QModelIndex &idx, list)
        if (idx.column() == 0)
            res.append(idx);
    return res;
}

void BreakWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete) {
        QItemSelectionModel *sm = selectionModel();
        QTC_ASSERT(sm, return);
        QModelIndexList si = sm->selectedIndexes();
        if (si.isEmpty())
            si.append(currentIndex().sibling(currentIndex().row(), 0));
        deleteBreakpoints(normalizeIndexes(si));
    }
    QTreeView::keyPressEvent(ev);
}

void BreakWindow::resizeEvent(QResizeEvent *ev)
{
    QTreeView::resizeEvent(ev);
}

void BreakWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    const QAbstractItemModel *itemModel = model();
    QItemSelectionModel *sm = selectionModel();
    QTC_ASSERT(sm, return);
    QModelIndexList si = sm->selectedIndexes();
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (si.isEmpty() && indexUnderMouse.isValid())
        si.append(indexUnderMouse.sibling(indexUnderMouse.row(), 0));
    si = normalizeIndexes(si);

    const int rowCount = itemModel->rowCount();
    const unsigned engineCapabilities = m_manager->debuggerCapabilities();

    QAction *deleteAction = new QAction(tr("Delete Breakpoint"), &menu);
    deleteAction->setEnabled(si.size() > 0);

    QAction *deleteAllAction = new QAction(tr("Delete All Breakpoints"), &menu);
    deleteAllAction->setEnabled(si.size() > 0);

    // Delete by file: Find indices of breakpoints of the same file.
    QAction *deleteByFileAction = 0;
    QList<int> breakPointsOfFile;
    if (indexUnderMouse.isValid()) {
        const QModelIndex index = indexUnderMouse.sibling(indexUnderMouse.row(), 2);
        const QString file = itemModel->data(index).toString();
        if (!file.isEmpty()) {
            for (int i = 0; i < rowCount; i++)
                if (itemModel->data(itemModel->index(i, 2)).toString() == file)
                    breakPointsOfFile.push_back(i);
            if (breakPointsOfFile.size() > 1) {
                deleteByFileAction =
                    new QAction(tr("Delete Breakpoints of \"%1\"").arg(file), &menu);
                deleteByFileAction->setEnabled(true);
            }
        }
    }
    if (!deleteByFileAction) {
        deleteByFileAction = new QAction(tr("Delete Breakpoints of File"), &menu);
        deleteByFileAction->setEnabled(false);
    }

    QAction *adjustColumnAction =
        new QAction(tr("Adjust Column Widths to Contents"), &menu);

    QAction *alwaysAdjustAction =
        new QAction(tr("Always Adjust Column Widths to Contents"), &menu);

    alwaysAdjustAction->setCheckable(true);
    alwaysAdjustAction->setChecked(m_alwaysResizeColumnsToContents);

    QAction *editConditionAction =
        new QAction(tr("Edit Condition..."), &menu);
    editConditionAction->setEnabled(si.size() > 0);

    QAction *synchronizeAction =
        new QAction(tr("Synchronize Breakpoints"), &menu);
    synchronizeAction->setEnabled(
        Debugger::DebuggerManager::instance()->debuggerActionsEnabled());

    QModelIndex idx0 = (si.size() ? si.front() : QModelIndex());
    QModelIndex idx2 = idx0.sibling(idx0.row(), 2);
    bool enabled = si.isEmpty() || itemModel->data(idx0, Qt::UserRole).toBool();
    const QString str5 = enabled ? tr("Disable Breakpoint") : tr("Enable Breakpoint");
    QAction *toggleEnabledAction = new QAction(str5, &menu);
    toggleEnabledAction->setEnabled(si.size() > 0);

    const bool fullpath = si.isEmpty() || itemModel->data(idx2, Qt::UserRole).toBool();
    const QString str6 = fullpath ? tr("Use Short Path") : tr("Use Full Path");
    QAction *pathAction = new QAction(str6, &menu);
    pathAction->setEnabled(si.size() > 0);

    QAction *breakAtFunctionAction =
        new QAction(tr("Set Breakpoint at Function..."), this);
    QAction *breakAtMainAction =
        new QAction(tr("Set Breakpoint at Function \"main\""), this);
    QAction *breakAtThrowAction =
        new QAction(tr("Set Breakpoint at \"throw\""), this);
    QAction *breakAtCatchAction =
        new QAction(tr("Set Breakpoint at \"catch\""), this);

    menu.addAction(deleteAction);
    menu.addAction(editConditionAction);
    menu.addAction(toggleEnabledAction);
    menu.addAction(pathAction);
    menu.addSeparator();
    menu.addAction(deleteAllAction);
    menu.addAction(deleteByFileAction);
    menu.addSeparator();
    menu.addAction(synchronizeAction);
    menu.addSeparator();
    menu.addAction(breakAtFunctionAction);
    menu.addAction(breakAtMainAction);
    if (engineCapabilities & BreakOnThrowAndCatchCapability) {
        menu.addAction(breakAtThrowAction);
        menu.addAction(breakAtCatchAction);
    }
    menu.addSeparator();
    menu.addAction(theDebuggerAction(UseToolTipsInBreakpointsView));
    menu.addAction(theDebuggerAction(UseAddressInBreakpointsView));
    menu.addAction(adjustColumnAction);
    menu.addAction(alwaysAdjustAction);
    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == deleteAction) {
        deleteBreakpoints(si);
    } else if (act == deleteAllAction) {
        QList<int> allRows;
        for (int i = 0; i < rowCount; i++)
            allRows.push_back(i);
        deleteBreakpoints(allRows);
    }  else if (act == deleteByFileAction)
        deleteBreakpoints(breakPointsOfFile);
    else if (act == adjustColumnAction)
        resizeColumnsToContents();
    else if (act == alwaysAdjustAction)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == editConditionAction)
        editConditions(si);
    else if (act == synchronizeAction)
        emit breakpointSynchronizationRequested();
    else if (act == toggleEnabledAction)
        setBreakpointsEnabled(si, !enabled);
    else if (act == pathAction)
        setBreakpointsFullPath(si, !fullpath);
    else if (act == breakAtFunctionAction) {
        BreakByFunctionDialog dlg(this);
        if (dlg.exec())
            emit breakByFunctionRequested(dlg.functionName());
    } else if (act == breakAtMainAction)
        emit breakByFunctionMainRequested();
     else if (act == breakAtThrowAction)
        emit breakByFunctionRequested("__cxa_throw");
     else if (act == breakAtCatchAction)
        emit breakByFunctionRequested("__cxa_begin_catch");
}

void BreakWindow::setBreakpointsEnabled(const QModelIndexList &list, bool enabled)
{
    foreach (const QModelIndex &idx, list)
        model()->setData(idx, enabled);
    emit breakpointSynchronizationRequested();
}

void BreakWindow::setBreakpointsFullPath(const QModelIndexList &list, bool fullpath)
{
    foreach (const QModelIndex &idx, list) {
        QModelIndex idx2 = idx.sibling(idx.row(), 2);
        model()->setData(idx2, fullpath);
    }
    emit breakpointSynchronizationRequested();
}

void BreakWindow::deleteBreakpoints(const QModelIndexList &indexes)
{
    QTC_ASSERT(!indexes.isEmpty(), return);
    QList<int> list;
    foreach (const QModelIndex &idx, indexes)
        list.append(idx.row());
    deleteBreakpoints(list);
}

void BreakWindow::deleteBreakpoints(QList<int> list)
{
    if (list.empty())
        return;
    const int firstRow = list.front();
    qSort(list.begin(), list.end());
    for (int i = list.size(); --i >= 0; )
        emit breakpointDeleted(list.at(i));

    const int row = qMin(firstRow, model()->rowCount() - 1);
    if (row >= 0)
        setCurrentIndex(model()->index(row, 0));
}

void BreakWindow::editConditions(const QModelIndexList &list)
{
    QDialog dlg(this);
    Ui::BreakCondition ui;
    ui.setupUi(&dlg);

    QTC_ASSERT(!list.isEmpty(), return);
    QModelIndex idx = list.front();
    int row = idx.row();
    dlg.setWindowTitle(tr("Conditions on Breakpoint %1").arg(row));
    ui.lineEditCondition->setText(model()->data(idx.sibling(row, 4)).toString());
    ui.spinBoxIgnoreCount->setValue(model()->data(idx.sibling(row, 5)).toInt());

    if (dlg.exec() == QDialog::Rejected)
        return;

    foreach (const QModelIndex &idx, list) {
        model()->setData(idx.sibling(idx.row(), 4), ui.lineEditCondition->text());
        model()->setData(idx.sibling(idx.row(), 5), ui.spinBoxIgnoreCount->value());
    }
}

void BreakWindow::resizeColumnsToContents()
{
    for (int i = model()->columnCount(); --i >= 0; )
        resizeColumnToContents(i);
}

void BreakWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    for (int i = model()->columnCount(); --i >= 0; )
        header()->setResizeMode(i, mode);
}

void BreakWindow::rowActivated(const QModelIndex &idx)
{
    emit breakpointActivated(idx.row());
}

