/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>

namespace Debugger {
namespace Internal {

BaseWindow::BaseWindow(QWidget *parent)
    : QTreeView(parent)
{
    QAction *act = debuggerCore()->action(UseAlternatingRowColors);

    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setUniformRowHeights(true);

    header()->setDefaultAlignment(Qt::AlignLeft);

    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivatedHelper(QModelIndex)));

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
    QTreeView::setModel(model);
    if (header() && m_alwaysAdjustColumnsAction)
        setAlwaysResizeColumnsToContents(m_alwaysAdjustColumnsAction->isChecked());
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
    header()->setResizeMode(0, mode);
}

void BaseWindow::reset()
{
    QTreeView::reset();
    if (header() && m_alwaysAdjustColumnsAction
            && m_alwaysAdjustColumnsAction->isChecked())
        resizeColumnsToContents();
}

} // namespace Internal
} // namespace Debugger
