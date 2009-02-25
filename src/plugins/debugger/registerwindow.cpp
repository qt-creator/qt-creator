/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "registerwindow.h"

#include "debuggerconstants.h"

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QHeaderView>
#include <QMenu>
#include <QResizeEvent>
#include <QToolButton>


using namespace Debugger::Internal;
using namespace Debugger::Constants;

RegisterWindow::RegisterWindow()
    : m_alwaysResizeColumnsToContents(true), m_alwaysReloadContents(false)
{
    setWindowTitle(tr("Registers"));
    setSortingEnabled(true);
    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    //header()->hide();
    //setIconSize(QSize(10, 10));
    //setWindowIcon(QIcon(":/gdbdebugger/images/debugger_breakpoints.png"));
    //QHeaderView *hv = header();
    //hv->setDefaultAlignment(Qt::AlignLeft);
    //hv->setClickable(true);
    //hv->setSortIndicatorShown(true);
}

void RegisterWindow::resizeEvent(QResizeEvent *ev)
{
    //QHeaderView *hv = header();
    //int totalSize = ev->size().width() - 110;
    //hv->resizeSection(0, totalSize / 4);
    //hv->resizeSection(1, totalSize / 4);
    QTreeView::resizeEvent(ev);
}

void RegisterWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    enum { Hex, Bin, Dec, Raw, Oct, Nat,
         Adjust, AlwaysAdjust, Reload, AlwaysReload, Count };

    QMenu menu;
    QAction *actions[Count];
    //QTreeWidgetItem *item = itemAt(ev->pos());
    QString format = model()->property(PROPERTY_REGISTER_FORMAT).toString();
    qDebug() << "FORMAT: " << format;

    actions[Adjust] = menu.addAction("Adjust column widths to contents");

    actions[AlwaysAdjust] = menu.addAction("Always adjust column widths to contents");
    actions[AlwaysAdjust]->setCheckable(true);
    actions[AlwaysAdjust]->setChecked(m_alwaysResizeColumnsToContents);

    actions[Reload] = menu.addAction("Reload register listing");

    actions[AlwaysReload] = menu.addAction("Always reload register listing");
    actions[AlwaysReload]->setCheckable(true);
    actions[AlwaysReload]->setChecked(m_alwaysReloadContents);

    menu.addSeparator();

    actions[Hex] = menu.addAction("Hexadecimal");
    actions[Hex]->setCheckable(true);
    actions[Hex]->setChecked(format == "h");

    actions[Bin] = menu.addAction("Binary");
    actions[Bin]->setCheckable(true);
    actions[Bin]->setChecked(format == "t");

    actions[Dec] = menu.addAction("Decimal");
    actions[Dec]->setCheckable(true);
    actions[Dec]->setChecked(format == "d");

    actions[Raw] = menu.addAction("Raw");
    actions[Raw]->setCheckable(true);
    actions[Raw]->setChecked(format == "r");

    actions[Nat] = menu.addAction("Natural");
    actions[Nat]->setCheckable(true);
    actions[Nat]->setChecked(format == "N");

    actions[Oct] = menu.addAction("Octal");
    actions[Oct]->setCheckable(true);
    actions[Oct]->setChecked(format == "o");

    QAction *act = menu.exec(ev->globalPos());

    if (act == actions[Adjust])
        resizeColumnsToContents();
    else if (act == actions[AlwaysAdjust])
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == actions[Reload])
        reloadContents();
    else if (act == actions[AlwaysReload])
        setAlwaysReloadContents(!m_alwaysReloadContents);
    else if (act == actions[Hex])
        model()->setProperty(PROPERTY_REGISTER_FORMAT, "h");
    else if (act == actions[Oct])
        model()->setProperty(PROPERTY_REGISTER_FORMAT, "o");
    else if (act == actions[Bin])
        model()->setProperty(PROPERTY_REGISTER_FORMAT, "t");
    else if (act == actions[Dec])
        model()->setProperty(PROPERTY_REGISTER_FORMAT, "d");
    else if (act == actions[Nat])
        model()->setProperty(PROPERTY_REGISTER_FORMAT, "N");
    
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
    
