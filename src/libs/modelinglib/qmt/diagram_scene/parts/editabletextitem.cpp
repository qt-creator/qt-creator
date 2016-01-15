/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "editabletextitem.h"

#include <QStyleOptionGraphicsItem>
#include <QKeyEvent>
#include <QTextCursor>
#include <QGraphicsScene>

namespace qmt {

EditableTextItem::EditableTextItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent)
{
    setTextInteractionFlags(Qt::TextEditorInteraction);
}

EditableTextItem::~EditableTextItem()
{
}

void EditableTextItem::setShowFocus(bool showFocus)
{
    m_showFocus = showFocus;
}

void EditableTextItem::setFilterReturnKey(bool filterReturnKey)
{
    m_filterReturnKey = filterReturnKey;
}

void EditableTextItem::setFilterTabKey(bool filterTabKey)
{
    m_filterTabKey = filterTabKey;
    // TODO due to a bug in Qt the tab key is never seen by keyPressEvent(). Use this as a workaround
    setTabChangesFocus(filterTabKey);
}

void EditableTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem option2(*option);
    if (!m_showFocus)
        option2.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
    QGraphicsTextItem::paint(painter, &option2, widget);
}

void EditableTextItem::selectAll()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    setTextCursor(cursor);
}

void EditableTextItem::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && m_filterReturnKey) {
        event->accept();
        emit returnKeyPressed();
    } else if (event->key() == Qt::Key_Tab && m_filterTabKey) {
        event->accept();
    } else {
        QGraphicsTextItem::keyPressEvent(event);
    }
}

void EditableTextItem::keyReleaseEvent(QKeyEvent *event)
{
    if (((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && m_filterReturnKey)
            || (event->key() == Qt::Key_Tab && m_filterTabKey))
        event->accept();
    else
        QGraphicsTextItem::keyReleaseEvent(event);
}

void EditableTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    scene()->clearSelection();
    parentItem()->setSelected(true);
    QGraphicsTextItem::mousePressEvent(event);
}

void EditableTextItem::focusOutEvent(QFocusEvent *event)
{
    QGraphicsTextItem::focusOutEvent(event);

    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

} // namespace qmt
