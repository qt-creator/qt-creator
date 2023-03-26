// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstracttool.h"

#include <QCoreApplication>
#include <QPointF>

QT_FORWARD_DECLARE_CLASS(QGraphicsRectItem)

namespace QmlDesigner {

class TimelineMovableAbstractItem;

class TimelineMoveTool : public TimelineAbstractTool
{
    Q_DECLARE_TR_FUNCTIONS(TimelineMoveTool)

public:
    explicit TimelineMoveTool(AbstractScrollGraphicsScene *scene, TimelineToolDelegate *delegate);
    void mousePressEvent(TimelineMovableAbstractItem *item,
                         QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(TimelineMovableAbstractItem *item,
                           QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                               QGraphicsSceneMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

private:
    qreal m_pressKeyframeDelta = 0.;
};

} // namespace QmlDesigner
