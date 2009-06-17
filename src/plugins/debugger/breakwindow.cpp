/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "breakwindow.h"

#include "debuggeractions.h"
#include "ui_breakcondition.h"
#include "ui_breakbyfunction.h"

#include <utils/qtcassert.h>

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

BreakWindow::BreakWindow(QWidget *parent)
  : QTreeView(parent), m_alwaysResizeColumnsToContents(false)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
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
    QItemSelectionModel *sm = selectionModel();
    QTC_ASSERT(sm, return);
    QModelIndexList si = sm->selectedIndexes();
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (si.isEmpty() && indexUnderMouse.isValid())
        si.append(indexUnderMouse.sibling(indexUnderMouse.row(), 0));
    si = normalizeIndexes(si);

    QAction *act0 = new QAction(tr("Delete breakpoint", 0, si.size()), &menu);
    act0->setEnabled(si.size() > 0);

    QAction *act1 = new QAction(tr("Adjust column widths to contents"), &menu);

    QAction *act2 = new QAction(tr("Always adjust column widths to contents"), &menu);
    act2->setCheckable(true);
    act2->setChecked(m_alwaysResizeColumnsToContents);

    QAction *act3 = new QAction(tr("Edit condition...", 0, si.size()), &menu);
    act3->setEnabled(si.size() > 0);

    QAction *act4 = new QAction(tr("Synchronize breakpoints"), &menu);

    QModelIndex idx0 = (si.size() ? si.front() : QModelIndex());
    QModelIndex idx2 = idx0.sibling(idx0.row(), 2);
    bool enabled = si.isEmpty() || model()->data(idx0, Qt::UserRole).toBool();
    QString str5 = enabled ? tr("Disable breakpoint") : tr("Enable breakpoint");
    QAction *act5 = new QAction(str5, &menu);
    act5->setEnabled(si.size() > 0);

    bool fullpath = si.isEmpty() || model()->data(idx2, Qt::UserRole).toBool();
    QString str6 = fullpath ? tr("Use short path") : tr("Use full path");
    QAction *act6 = new QAction(str6, &menu);
    act6->setEnabled(si.size() > 0);

    QAction *act7 = new QAction(this);
    act7->setText(tr("Set Breakpoint at Function..."));
    QAction *act8 = new QAction(this);
    act8->setText(tr("Set Breakpoint at Function \"main\""));

    menu.addAction(act0);
    menu.addAction(act3);
    menu.addAction(act5);
    menu.addAction(act6);
    menu.addSeparator();
    menu.addAction(act1);
    menu.addAction(act2);
    menu.addAction(act4);
    menu.addSeparator();
    menu.addAction(act7);
    menu.addAction(act8);
    menu.addSeparator();
    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == act0)
        deleteBreakpoints(si);
    else if (act == act1)
        resizeColumnsToContents();
    else if (act == act2)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == act3)
        editConditions(si);
    else if (act == act4)
        emit breakpointSynchronizationRequested();
    else if (act == act5)
        setBreakpointsEnabled(si, !enabled);
    else if (act == act6)
        setBreakpointsFullPath(si, !enabled);
    else if (act == act7) {
        BreakByFunctionDialog dlg(this);
        if (dlg.exec())
            emit breakByFunctionRequested(dlg.functionName());
    } else if (act == act8)
        emit breakByFunctionMainRequested();
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

    qSort(list.begin(), list.end());
    for (int i = list.size(); --i >= 0; )
        emit breakpointDeleted(i);

    QModelIndex idx = indexes.front();
    int row = qMax(idx.row(), model()->rowCount() - list.size() - 1);
    setCurrentIndex(idx.sibling(row, 0));
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
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

void BreakWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on 
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
    header()->setResizeMode(2, mode);
    header()->setResizeMode(3, mode);
}

void BreakWindow::rowActivated(const QModelIndex &idx)
{
    emit breakpointActivated(idx.row());
}

