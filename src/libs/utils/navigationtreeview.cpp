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

#include "navigationtreeview.h"

#include <QtGui/QHeaderView>
#include <QtGui/QFocusEvent>

#ifdef Q_WS_MAC
#include <QtGui/QKeyEvent>
#endif

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

    // show horizontal scrollbar
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

#ifdef Q_WS_MAC
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
