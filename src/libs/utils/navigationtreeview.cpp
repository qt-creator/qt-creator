/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "navigationtreeview.h"

#include <QHeaderView>
#include <QFocusEvent>

#ifdef Q_OS_MAC
#include <QKeyEvent>
#endif

/*!
   \class Utils::NavigationTreeView

   \brief General TreeView for any Side Bar widget.

   Common initialization etc, e.g. Mac specific behaviour.
   \sa Core::NavigationView, Core::INavigationWidgetFactory
 */

namespace Utils {

NavigationTreeView::NavigationTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setIndentation(indentation() * 9/10);
    setUniformRowHeights(true);
    setTextElideMode(Qt::ElideNone);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    setHeaderHidden(true);
    // We let the column adjust to contents, but note
    // the setting of a minimum size in resizeEvent()
    header()->setResizeMode(QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
}

// This is a workaround to stop Qt from redrawing the project tree every
// time the user opens or closes a menu when it has focus. Would be nicer to
// fix it in Qt.
void NavigationTreeView::focusInEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::PopupFocusReason)
        QTreeView::focusInEvent(event);
}

void NavigationTreeView::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::PopupFocusReason)
        QTreeView::focusOutEvent(event);
}

void NavigationTreeView::resizeEvent(QResizeEvent *event)
{
    header()->setMinimumSectionSize(viewport()->width());
    QTreeView::resizeEvent(event);
}

#ifdef Q_OS_MAC
void NavigationTreeView::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter)
            && event->modifiers() == 0
            && currentIndex().isValid()
            && state() != QAbstractItemView::EditingState) {
        emit activated(currentIndex());
        return;
    }
    QTreeView::keyPressEvent(event);
}
#endif

} // namespace Utils
