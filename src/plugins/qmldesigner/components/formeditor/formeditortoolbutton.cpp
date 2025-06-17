// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditortoolbutton.h"
#include "formeditortracing.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QAction>
#include <QCursor>

namespace QmlDesigner {

using FormEditorTracing::category;

const int toolButtonSize = 14;

FormEditorToolButton::FormEditorToolButton(QAction *action, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , m_action(action)
{
    NanotraceHR::Tracer tracer{"form editor tool button constructor", category()};

    resize(toolButtonSize, toolButtonSize + 2);
    setPreferredSize(toolButtonSize, toolButtonSize + 2);
    setAcceptHoverEvents(true);
    connect(action, &QAction::changed, this, [this]() {
        setEnabled(m_action->isEnabled());
        setVisible(m_action->isVisible());
        // Reset to Normal. When action isn't enabled anymore and therefor the button too, it
        // doesn't receives events anymore, which leaves the state in Hover.
        m_state = Normal;
        update();
    });

    connect(this, &FormEditorToolButton::clicked, action, &QAction::trigger);

    setEnabled(m_action->isEnabled());
    setVisible(m_action->isVisible());
    setCursor(Qt::ArrowCursor);
}

void FormEditorToolButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    NanotraceHR::Tracer tracer{"form editor tool button paint", category()};

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF adjustedRect(size().width() - toolButtonSize,
                        size().height() - toolButtonSize,
                        toolButtonSize,
                        toolButtonSize);
    painter->setPen(Qt::NoPen);

    static QColor selectionColor = Utils::creatorColor(
        Utils::Theme::QmlDesigner_FormEditorSelectionColor);

    if (m_state == Hovered)
        painter->setBrush(selectionColor.lighter(110));
    else
        painter->setBrush(selectionColor.darker(120));

    if (m_state != Normal)
        painter->drawRoundedRect(adjustedRect, 1, 1, Qt::AbsoluteSize);

    if (!isEnabled())
        painter->setOpacity(0.5);

    painter->drawPixmap(size().width() - toolButtonSize,
                        size().height() - toolButtonSize,
                        m_action->icon().pixmap(toolButtonSize, toolButtonSize));

    painter->restore();
}

QRectF FormEditorToolButton::boundingRect() const
{
    NanotraceHR::Tracer tracer{"form editor tool button bounding rect", category()};

    return QRectF(0, 0, toolButtonSize, toolButtonSize + 2);
}

void FormEditorToolButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor tool button hover enter event", category()};

    m_state = Hovered;

    QGraphicsObject::hoverEnterEvent(event);
    event->accept();
    update();
}

void FormEditorToolButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor tool button hover leave event", category()};

    m_state = Normal;

    QGraphicsWidget::hoverLeaveEvent(event);
    event->accept();
    update();
}

void FormEditorToolButton::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor tool button hover move event", category()};

    QGraphicsWidget::hoverMoveEvent(event);
}

void FormEditorToolButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor tool button mouse press event", category()};

    m_state = Pressed;
    event->accept();
    update();
}

void FormEditorToolButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor tool button mouse release event", category()};

    m_state = Hovered;

    event->accept();
    emit clicked();
}

} // namespace QmlDesigner
