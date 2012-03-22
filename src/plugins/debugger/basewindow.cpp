/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "basewindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"

#include <aggregation/aggregate.h>
#include <coreplugin/findplaceholder.h>
#include <find/treeviewfind.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QContextMenuEvent>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QVBoxLayout>

namespace Debugger {
namespace Internal {

BaseWindow::BaseWindow(QWidget *parent)
    : QWidget(parent)
{
    QAction *act = debuggerCore()->action(UseAlternatingRowColors);

    m_treeView = new QTreeView(this);
    m_treeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_treeView->setFrameStyle(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(act->isChecked());
    m_treeView->setRootIsDecorated(false);
    m_treeView->setIconSize(QSize(10, 10));
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setUniformRowHeights(true);

    m_treeView->header()->setDefaultAlignment(Qt::AlignLeft);
    m_treeView->header()->setClickable(true);

    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(m_treeView, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivatedHelper(QModelIndex)));
    connect(m_treeView->header(), SIGNAL(sectionClicked(int)),
        SLOT(headerSectionClicked(int)));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    vbox->setSpacing(0);
    vbox->addWidget(m_treeView);
    vbox->addWidget(new Core::FindToolBarPlaceHolder(this));

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_treeView);
    agg->add(new Find::TreeViewFind(m_treeView));

    m_adjustColumnsAction = new QAction(tr("Adjust Column Widths to Contents"), 0);
    m_alwaysAdjustColumnsAction = 0;
}

void BaseWindow::setAlwaysAdjustColumnsAction(QAction *action)
{
    m_alwaysAdjustColumnsAction = action;
    connect(action, SIGNAL(toggled(bool)),
        SLOT(setAlwaysResizeColumnsToContents(bool)));
}

void BaseWindow::addBaseContextActions(QMenu *menu)
{
    menu->addSeparator();
    if (m_alwaysAdjustColumnsAction)
        menu->addAction(m_alwaysAdjustColumnsAction);
    menu->addAction(m_adjustColumnsAction);
    menu->addSeparator();
    menu->addAction(debuggerCore()->action(SettingsDialog));
}

bool BaseWindow::handleBaseContextAction(QAction *act)
{
    if (act == 0)
        return true;
    if (act == m_adjustColumnsAction) {
        resizeColumnsToContents();
        return true;
    }
    if (act == m_alwaysAdjustColumnsAction) {
        if (act->isChecked())
            resizeColumnsToContents();
        // Action triggered automatically.
        return true;
    }
    return false;
}

void BaseWindow::setModel(QAbstractItemModel *model)
{
    m_treeView->setModel(model);
    if (m_treeView->header() && m_alwaysAdjustColumnsAction)
        setAlwaysResizeColumnsToContents(m_alwaysAdjustColumnsAction->isChecked());
}

void BaseWindow::mousePressEvent(QMouseEvent *ev)
{
    QWidget::mousePressEvent(ev);
    if (!m_treeView->indexAt(ev->pos()).isValid())
        resizeColumnsToContents();
}

void BaseWindow::resizeColumnsToContents()
{
    const int columnCount = model()->columnCount();
    for (int c = 0 ; c != columnCount; ++c)
        resizeColumnToContents(c);
}

void BaseWindow::setAlwaysResizeColumnsToContents(bool on)
{
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    m_treeView->header()->setResizeMode(0, mode);
}

void BaseWindow::setAlternatingRowColorsHelper(bool on)
{
    m_treeView->setAlternatingRowColors(on);
}

void BaseWindow::rowActivatedHelper(const QModelIndex &index)
{
    rowActivated(index);
}

void BaseWindow::headerSectionClicked(int logicalIndex)
{
    resizeColumnToContents(logicalIndex);
}

void BaseWindow::reset()
{
    m_treeView->reset();
    if (m_treeView->header() && m_alwaysAdjustColumnsAction
            && m_alwaysAdjustColumnsAction->isChecked())
        resizeColumnsToContents();
}

QModelIndexList BaseWindow::selectedIndices(QContextMenuEvent *ev)
{
    QItemSelectionModel *sm = treeView()->selectionModel();
    QTC_ASSERT(sm, return QModelIndexList());
    QModelIndexList si = sm->selectedIndexes();
    if (ev) {
        QModelIndex indexUnderMouse = m_treeView->indexAt(ev->pos());
        if (si.isEmpty() && indexUnderMouse.isValid())
            si.append(indexUnderMouse);
    }
    return si;
}

} // namespace Internal
} // namespace Debugger
