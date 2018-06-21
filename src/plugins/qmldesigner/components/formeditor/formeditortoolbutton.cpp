/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "formeditortoolbutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include <utils/theme/theme.h>
#include <utils/stylehelper.h>

#include <QAction>

namespace QmlDesigner {

const int toolButtonSize = 14;

FormEditorToolButton::FormEditorToolButton(QAction *action, QGraphicsItem *parent)  : QGraphicsWidget(parent), m_action(action)
{
    resize(toolButtonSize, toolButtonSize + 2);
    setPreferredSize(toolButtonSize, toolButtonSize + 2);
    setAcceptHoverEvents(true);
    connect(action, &QAction::changed, this, [this](){
        setEnabled(m_action->isEnabled());
        setVisible(m_action->isVisible());
        update();
    });

    connect(this, &FormEditorToolButton::clicked, action, &QAction::trigger);

    setEnabled(m_action->isEnabled());
    setVisible(m_action->isVisible());
    setCursor(Qt::ArrowCursor);
}

void FormEditorToolButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF adjustedRect(size().width() - toolButtonSize, size().height() - toolButtonSize, toolButtonSize, toolButtonSize);
    painter->setPen(Qt::NoPen);

    static QColor selectionColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorSelectionColor);

    if (m_state == Hovered)
        painter->setBrush(selectionColor.lighter(110));
    else
        painter->setBrush(selectionColor.darker(120));

    if (m_state != Normal)
        painter->drawRoundedRect(adjustedRect, 1, 1, Qt::AbsoluteSize);

    if (!isEnabled())
        setOpacity(0.5);

    painter->drawPixmap(size().width() - toolButtonSize, size().height() - toolButtonSize, m_action->icon().pixmap(toolButtonSize, toolButtonSize));

    painter->restore();
}

QRectF FormEditorToolButton::boundingRect() const
{
    return QRectF(0, 0, toolButtonSize, toolButtonSize + 2);
}

void FormEditorToolButton::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    m_state = Hovered;

    QGraphicsObject::hoverEnterEvent(event);
    event->accept();
    update();
}

void FormEditorToolButton::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    m_state = Normal;

    QGraphicsWidget::hoverLeaveEvent(event);
    event->accept();
    update();
}

void FormEditorToolButton::hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
    QGraphicsWidget::hoverMoveEvent(event);
}

void FormEditorToolButton::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    m_state = Pressed;
    event->accept();
    update();
}

void FormEditorToolButton::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    m_state = Hovered;

    event->accept();
    emit clicked();
}

} //QmlDesigner
