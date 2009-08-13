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

#include "registerwindow.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFileInfoList>

#include <QtGui/QAction>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>
#include <QtGui/QToolButton>


using namespace Debugger::Internal;
using namespace Debugger::Constants;

RegisterWindow::RegisterWindow()
  : m_alwaysResizeColumnsToContents(true), m_alwaysReloadContents(false)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Registers"));
    setSortingEnabled(true);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);

    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

void RegisterWindow::resizeEvent(QResizeEvent *ev)
{
    QTreeView::resizeEvent(ev);
}

void RegisterWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;

    QAction *actAdjust = menu.addAction(tr("Adjust column widths to contents"));
    QAction *actAlwaysAdjust =
        menu.addAction(tr("Always adjust column widths to contents"));
    actAlwaysAdjust->setCheckable(true);
    actAlwaysAdjust->setChecked(m_alwaysResizeColumnsToContents);

    QAction *actReload = menu.addAction(tr("Reload register listing"));
    QAction *actAlwaysReload = menu.addAction(tr("Always reload register listing"));
    actAlwaysReload->setCheckable(true);
    actAlwaysReload->setChecked(m_alwaysReloadContents);
    menu.addSeparator();

    int base = model()->data(QModelIndex(), Qt::UserRole).toInt();
    QAction *act16 = menu.addAction(tr("Hexadecimal"));
    act16->setCheckable(true);
    act16->setChecked(base == 16);
    QAction *act10 = menu.addAction(tr("Decimal"));
    act10->setCheckable(true);
    act10->setChecked(base == 10);
    QAction *act8 = menu.addAction(tr("Octal"));
    act8->setCheckable(true);
    act8->setChecked(base == 8);
    QAction *act2 = menu.addAction(tr("Binary"));
    act2->setCheckable(true);
    act2->setChecked(base == 2);
    menu.addSeparator();

    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());
    
    if (act == actAdjust)
        resizeColumnsToContents();
    else if (act == actAlwaysAdjust)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == actReload)
        reloadContents();
    else if (act == actAlwaysReload)
        setAlwaysReloadContents(!m_alwaysReloadContents);
    else if (act) {
        base = (act == act10 ? 10 : act == act8 ? 8 : act == act2 ? 2 : 16);
        QMetaObject::invokeMethod(model(), "setNumberBase", Q_ARG(int, base));
    }
}

void RegisterWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void RegisterWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
}

void RegisterWindow::setAlwaysReloadContents(bool on)
{
    m_alwaysReloadContents = on;
    if (m_alwaysReloadContents)
        reloadContents();
}

void RegisterWindow::reloadContents()
{
    emit reloadRegisterRequested();
}


void RegisterWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setAlwaysResizeColumnsToContents(true);
}
    
