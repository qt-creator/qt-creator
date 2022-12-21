// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstractrenderer.h"

namespace Timeline {

class TRACING_EXPORT TimelineOverviewRenderer : public TimelineAbstractRenderer
{
    Q_OBJECT
    QML_ELEMENT

public:
    TimelineOverviewRenderer(QQuickItem *parent = nullptr);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

    class TimelineOverviewRendererPrivate;
    Q_DECLARE_PRIVATE(TimelineOverviewRenderer)
};

} // namespace Timeline

QML_DECLARE_TYPE(Timeline::TimelineOverviewRenderer)
