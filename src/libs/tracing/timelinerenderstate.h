// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QSGNode>
#include "timelinerenderpass.h"
#include "timelinemodel.h"

namespace Timeline {

class TRACING_EXPORT TimelineRenderState {
public:
    TimelineRenderState(qint64 start, qint64 end, float scale, int numPasses);
    ~TimelineRenderState();

    qint64 start() const;
    qint64 end() const;
    float scale() const;

    TimelineRenderPass::State *passState(int i);
    const TimelineRenderPass::State *passState(int i) const;
    void setPassState(int i, TimelineRenderPass::State *state);

    const QSGNode *expandedRowRoot() const;
    const QSGNode *collapsedRowRoot() const;
    const QSGNode *expandedOverlayRoot() const;
    const QSGNode *collapsedOverlayRoot() const;

    QSGNode *expandedRowRoot();
    QSGNode *collapsedRowRoot();
    QSGNode *expandedOverlayRoot();
    QSGNode *collapsedOverlayRoot();

    bool isEmpty() const;
    void assembleNodeTree(const TimelineModel *model, int defaultRowHeight, int defaultRowOffset);
    void updateExpandedRowHeights(const TimelineModel *model, int defaultRowHeight,
                                  int defaultRowOffset);
    QSGTransformNode *finalize(QSGNode *oldNode, bool expanded, const QMatrix4x4 &transform);

private:
    QSGNode *m_expandedRowRoot;
    QSGNode *m_collapsedRowRoot;
    QSGNode *m_expandedOverlayRoot;
    QSGNode *m_collapsedOverlayRoot;
    qint64 m_start;
    qint64 m_end;
    float m_scale; // "native" scale, this stays the same through the life time of a state
    QList<TimelineRenderPass::State *> m_passes;
};

} // namespace Timeline
