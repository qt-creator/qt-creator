/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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
