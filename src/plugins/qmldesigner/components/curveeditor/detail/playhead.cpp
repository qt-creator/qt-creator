// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "playhead.h"
#include "curveeditormodel.h"
#include "graphicsview.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace QmlDesigner {

constexpr double g_playheadMargin = 5.0;

Playhead::Playhead(GraphicsView *view)
    : m_frame(0)
    , m_moving(false)
    , m_rect()
    , m_timer()
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(30);
    QObject::connect(&m_timer, &QTimer::timeout, view, [this, view]() {
        if (QApplication::mouseButtons() == Qt::LeftButton)
            mouseMoveOutOfBounds(view);
    });
}

int Playhead::currentFrame() const
{
    return m_frame;
}

void Playhead::setMoving(bool moving)
{
    m_moving = moving;
}

void Playhead::moveToFrame(int frame, GraphicsView *view)
{
    m_frame = frame;
    m_rect.moveCenter(QPointF(view->mapTimeToX(m_frame), m_rect.center().y()));
}

void Playhead::resize(GraphicsView *view)
{
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();

    CurveEditorStyle style = view->editorStyle();

    QPointF tlr(style.valueAxisWidth, style.timeAxisHeight - style.playhead.width);
    QPointF brr(style.valueAxisWidth + style.playhead.width, -g_playheadMargin);

    QPointF tl = viewRect.topLeft() + tlr;
    QPointF br = viewRect.bottomLeft() + brr;

    m_rect = QRectF(tl, br);

    moveToFrame(m_frame, view);
}

bool Playhead::mousePress(const QPointF &pos)
{
    QRectF hitRect = m_rect;
    hitRect.setBottom(hitRect.top() + hitRect.width());

    m_moving = hitRect.contains(pos);

    return m_moving;
}

bool Playhead::mouseMove(const QPointF &pos, GraphicsView *view)
{
    if (m_moving) {
        CurveEditorStyle style = view->editorStyle();

        QRectF canvas = view->canvasRect().adjusted(0.0, -style.timeAxisHeight, 0.0, 0.0);

        if (canvas.contains(pos)) {
            int frame = std::round(view->mapXtoTime(pos.x()));
            view->setCurrentFrame(frame);
        } else if (!m_timer.isActive())
            m_timer.start();
    }

    return m_moving;
}

void Playhead::mouseMoveOutOfBounds(GraphicsView *view)
{
    if (QApplication::mouseButtons() != Qt::LeftButton)
        return;

    CurveEditorStyle style = view->editorStyle();
    QRectF canvas = view->canvasRect();
    QPointF pos = view->globalToScene(QCursor::pos());

    if (pos.x() > canvas.right()) {
        double speed = (pos.x() - canvas.right());
        double nextCenter = m_rect.center().x() + speed;
        double frame = std::round(view->mapXtoTime(nextCenter));
        view->setCurrentFrame(frame);

        double framePosition = view->mapTimeToX(frame);
        double rightSideOut = framePosition + style.playhead.width / 2.0 + style.canvasMargin;
        double overshoot = rightSideOut - canvas.right();
        view->scrollContent(overshoot, 0.0);

    } else if (pos.x() < canvas.left()) {
        double speed = (canvas.left() - pos.x());
        double nextCenter = m_rect.center().x() - speed;
        double frame = std::round(view->mapXtoTime(nextCenter));
        view->setCurrentFrame(frame);

        double framePosition = view->mapTimeToX(frame);
        double leftSideOut = framePosition - style.playhead.width / 2.0 - style.canvasMargin;
        double undershoot = canvas.left() - leftSideOut;
        view->scrollContent(-undershoot, 0.0);
    }

    m_timer.start();
}

void Playhead::mouseRelease([[maybe_unused]] GraphicsView *view)
{
    m_moving = false;
}

void Playhead::paint(QPainter *painter, GraphicsView *view) const
{
    CurveEditorStyle style = view->editorStyle();

    painter->save();
    painter->setPen(style.playhead.color);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRectF rect = m_rect;
    rect.setBottom(m_rect.top() + m_rect.width());

    QPointF top(rect.center().x(), rect.top());
    QPointF right(rect.right(), rect.top());
    QPointF bottom(rect.center().x(), rect.bottom());
    QPointF left(rect.left(), rect.top());

    QLineF rightToBottom(right, bottom);
    rightToBottom.setLength(style.playhead.radius);

    QLineF leftToBottom(left, bottom);
    leftToBottom.setLength(style.playhead.radius);

    QPainterPath path(top);
    path.lineTo(right - QPointF(style.playhead.radius, 0.));
    path.quadTo(right, rightToBottom.p2());
    path.lineTo(bottom);
    path.lineTo(leftToBottom.p2());
    path.quadTo(left, left + QPointF(style.playhead.radius, 0.));
    path.closeSubpath();

    painter->fillPath(path, style.playhead.color);
    painter->drawLine(top + QPointF(0., 5.), QPointF(m_rect.center().x(), m_rect.bottom()));

    painter->restore();
}

} // End namespace QmlDesigner.
