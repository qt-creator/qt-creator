/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "timelinetoolbutton.h"

#include "timelineconstants.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QAction>
#include <QDebug>

namespace QmlDesigner {

TimelineToolButton::TimelineToolButton(QAction *action, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , m_action(action)
{
    resize(TimelineConstants::toolButtonSize, TimelineConstants::toolButtonSize);
    setPreferredSize(size());
    setAcceptHoverEvents(true);
    connect(action, &QAction::changed, this, [this]() {
        setVisible(m_action->isVisible());
        update();
    });

    connect(this, &TimelineToolButton::clicked, action, &QAction::trigger);

    setEnabled(m_action->isEnabled());
    setVisible(m_action->isVisible());
    setCursor(Qt::ArrowCursor);
}

void TimelineToolButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->save();

    if (m_state == Normal)
        setOpacity(0.8);
    else if (m_state == Pressed)
        setOpacity(0.3);
    else
        setOpacity(1.0);

    if (!isEnabled())
        setOpacity(0.5);

    if (isCheckable()) {
        if (isChecked() || isDisabled())
            m_action->icon().paint(painter,
                                   rect().toRect(),
                                   Qt::AlignCenter,
                                   QIcon::Normal,
                                   QIcon::On);
        else
            m_action->icon().paint(painter,
                                   rect().toRect(),
                                   Qt::AlignCenter,
                                   QIcon::Normal,
                                   QIcon::Off);
    } else
        m_action->icon().paint(painter, rect().toRect());

    painter->restore();
}

QRectF TimelineToolButton::boundingRect() const
{
    return QRectF(0, 0, TimelineConstants::toolButtonSize, TimelineConstants::toolButtonSize);
}

bool TimelineToolButton::isCheckable() const
{
    return m_action->isCheckable();
}

bool TimelineToolButton::isChecked() const
{
    return m_action->isChecked();
}

bool TimelineToolButton::isDisabled() const
{
    return !m_action->isEnabled();
}

void TimelineToolButton::setChecked(bool b)
{
    m_action->setChecked(b);
    update();
}

void TimelineToolButton::setDisabled(bool b)
{
    m_action->setDisabled(b);
}

void TimelineToolButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_state = Hovered;

    QGraphicsObject::hoverEnterEvent(event);
    event->accept();
    update();
}

void TimelineToolButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_state = Normal;

    QGraphicsWidget::hoverLeaveEvent(event);
    event->accept();
    update();
}

void TimelineToolButton::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    m_state = Hovered;
    QGraphicsWidget::hoverMoveEvent(event);
    update();
}

void TimelineToolButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_state = Pressed;
    event->accept();
    update();
}

void TimelineToolButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_state = Normal;

    event->accept();
    emit clicked();
}

} // namespace QmlDesigner
