// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    if (isReturnKey(event) && m_filterReturnKey) {
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
    if (isReturnKey(event) && m_filterReturnKey)
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

bool EditableTextItem::isReturnKey(QKeyEvent *event) const
{
    return (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            && (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier)) == 0;
}

} // namespace qmt
