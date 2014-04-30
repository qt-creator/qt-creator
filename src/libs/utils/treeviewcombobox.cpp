/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "treeviewcombobox.h"

#include <QWheelEvent>

using namespace Utils;

TreeViewComboBoxView::TreeViewComboBoxView(QWidget *parent)
    : QTreeView(parent)
{
    // TODO: Disable the root for all items (with a custom delegate?)
    setRootIsDecorated(false);
}

void TreeViewComboBoxView::adjustWidth(int width)
{
    setMaximumWidth(width);
    setMinimumWidth(qMin(qMax(sizeHintForColumn(0), minimumSizeHint().width()), width));
}


TreeViewComboBox::TreeViewComboBox(QWidget *parent)
    : QComboBox(parent), m_skipNextHide(false)
{
    m_view = new TreeViewComboBoxView;
    m_view->setHeaderHidden(true);
    m_view->setItemsExpandable(true);
    setView(m_view);
    m_view->viewport()->installEventFilter(this);
}

void TreeViewComboBox::wheelEvent(QWheelEvent *e)
{
    QModelIndex index = m_view->currentIndex();
    if (e->delta() > 0) {
        do
            index = m_view->indexAbove(index);
        while (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable));
    } else if (e->delta() < 0) {
        do
            index = m_view->indexBelow(index);
        while (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable));
    }
    e->accept();
    if (!index.isValid())
        return;

    setCurrentIndex(index);

    // for compatibility we emit activated with a useless row parameter
    emit activated(index.row());
}

void TreeViewComboBox::setCurrentIndex(const QModelIndex &index)
{
    setRootModelIndex(model()->parent(index));
    QComboBox::setCurrentIndex(index.row());
    setRootModelIndex(QModelIndex());
    m_view->setCurrentIndex(index);
}

bool TreeViewComboBox::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && object == view()->viewport()) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex index = view()->indexAt(mouseEvent->pos());
        if (!view()->visualRect(index).contains(mouseEvent->pos()))
            m_skipNextHide = true;
    }
    return false;
}

void TreeViewComboBox::showPopup()
{
    m_view->adjustWidth(topLevelWidget()->geometry().width());
    QComboBox::showPopup();
}

void TreeViewComboBox::hidePopup()
{
    if (m_skipNextHide)
        m_skipNextHide = false;
    else
        QComboBox::hidePopup();
}

TreeViewComboBoxView *TreeViewComboBox::view() const
{
    return m_view;
}
