/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

QModelIndex TreeViewComboBox::indexAbove(QModelIndex index)
{
    do
        index = m_view->indexAbove(index);
    while (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable));
    return index;
}

QModelIndex TreeViewComboBox::indexBelow(QModelIndex index)
{
    do
        index = m_view->indexBelow(index);
    while (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable));
    return index;
}

QModelIndex TreeViewComboBox::lastIndex(const QModelIndex &index)
{
    if (index.isValid() && !m_view->isExpanded(index))
        return index;

    int rows = m_view->model()->rowCount(index);
    if (rows == 0)
        return index;
    return lastIndex(m_view->model()->index(rows - 1, 0, index));
}

void TreeViewComboBox::wheelEvent(QWheelEvent *e)
{
    QModelIndex index = m_view->currentIndex();
    if (e->delta() > 0)
        index = indexAbove(index);
    else if (e->delta() < 0)
        index = indexBelow(index);

    e->accept();
    if (!index.isValid())
        return;

    setCurrentIndex(index);

    // for compatibility we emit activated with a useless row parameter
    emit activated(index.row());
}

void TreeViewComboBox::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_PageUp) {
        setCurrentIndex(indexAbove(m_view->currentIndex()));
    } else if (e->key() == Qt::Key_Down || e->key() == Qt::Key_PageDown) {
        setCurrentIndex(indexBelow(m_view->currentIndex()));
    } else if (e->key() == Qt::Key_Home) {
        QModelIndex index = m_view->model()->index(0, 0);
        if (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable))
            index = indexBelow(index);
        setCurrentIndex(index);
    } else if (e->key() == Qt::Key_End) {
        QModelIndex index = lastIndex(m_view->rootIndex());
        if (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable))
            index = indexAbove(index);
        setCurrentIndex(index);
    } else {
        QComboBox::keyPressEvent(e);
        return;
    }

    e->accept();
}

void TreeViewComboBox::setCurrentIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
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
