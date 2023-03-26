// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QRectF>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

namespace QmlDesigner {

class GraphicsView;

class Playhead
{
public:
    Playhead(GraphicsView *view);

    int currentFrame() const;

    void paint(QPainter *painter, GraphicsView *view) const;

    void setMoving(bool moving);

    void moveToFrame(int frame, GraphicsView *view);

    void resize(GraphicsView *view);

    bool mousePress(const QPointF &pos);

    bool mouseMove(const QPointF &pos, GraphicsView *view);

    void mouseRelease(GraphicsView *view);

private:
    void mouseMoveOutOfBounds(GraphicsView *view);

    int m_frame;

    bool m_moving;

    QRectF m_rect;

    QTimer m_timer;
};

} // End namespace QmlDesigner.
