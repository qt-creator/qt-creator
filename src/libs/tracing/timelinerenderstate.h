// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinerenderpass.h"
#include "timelinemodel.h"

#include <QSGNode>

namespace Timeline {

class TRACING_EXPORT TimelineRenderState
{
public:
    TimelineRenderState(qint64 start, qint64 end, float scale, int numPasses);
    ~TimelineRenderState();

    bool isEmpty() const;
    void assembleNodeTree(const TimelineModel *model, int defaultRowHeight, int defaultRowOffset);
    void updateExpandedRowHeights(const TimelineModel *model, int defaultRowHeight,
                                  int defaultRowOffset);
    QSGTransformNode *finalize(QSGNode *oldNode, bool expanded, const QMatrix4x4 &transform);

    QSGNode expandedRowRoot;
    QSGNode collapsedRowRoot;
    QSGNode expandedOverlayRoot;
    QSGNode collapsedOverlayRoot;

    const qint64 start;
    const qint64 end;
    const float scale; // "native" scale, this stays the same through the life time of a state

    QList<TimelineRenderPass::State *> passes;
};

} // namespace Timeline
