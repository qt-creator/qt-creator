// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinezoomcontrol.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"
#include "timelineabstractrenderer.h"

#include <QSGTransformNode>
#include <QtQml/qqml.h>

namespace Timeline {

class TRACING_EXPORT TimelineRenderer : public TimelineAbstractRenderer
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TimelineRenderer(QQuickItem *parent = nullptr);

    Q_INVOKABLE void selectNextFromSelectionId(int selectionId);
    Q_INVOKABLE void selectPrevFromSelectionId(int selectionId);
    Q_INVOKABLE void clearData();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    class TimelineRendererPrivate;
    Q_DECLARE_PRIVATE(TimelineRenderer)
};

} // namespace Timeline

QML_DECLARE_TYPE(Timeline::TimelineRenderer)
