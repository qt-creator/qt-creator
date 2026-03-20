// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineoverviewrenderer.h"
#include "timelinerenderstate.h"

namespace Timeline {

TimelineOverviewRenderer::TimelineOverviewRenderer(QQuickItem *parent)
    : TimelineAbstractRenderer(parent)
{
}

TimelineOverviewRenderer::~TimelineOverviewRenderer()
{
    delete m_renderState;
}

QSGNode *TimelineOverviewRenderer::updatePaintNode(QSGNode *oldNode,
                                                   UpdatePaintNodeData *updatePaintNodeData)
{
    if (!m_model || m_model->isEmpty() || !m_zoomer || m_zoomer->traceDuration() <= 0) {
        delete oldNode;
        return nullptr;
    }

    if (m_modelDirty) {
        delete m_renderState;
        m_renderState = nullptr;
    }

    if (m_renderState == nullptr) {
        m_renderState = new TimelineRenderState(m_zoomer->traceStart(), m_zoomer->traceEnd(),
                                                1.0, m_renderPasses.size());
    }

    float xSpacing = static_cast<float>(width() / m_zoomer->traceDuration());
    float ySpacing = static_cast<float>(
                height() / (m_model->collapsedRowCount() * TimelineModel::defaultRowHeight()));

    for (int i = 0; i < m_renderPasses.length(); ++i) {
        m_renderState->passes[i] = m_renderPasses[i]->update(this, m_renderState,
                                                              m_renderState->passes[i],
                                                              0, m_model->count(), true,
                                                              xSpacing);
    }

    if (m_renderState->isEmpty())
        m_renderState->assembleNodeTree(m_model, m_model->height(), 0);

    TimelineAbstractRenderer::updatePaintNode(nullptr, updatePaintNodeData);

    QMatrix4x4 matrix;
    matrix.scale(xSpacing, ySpacing, 1);
    return m_renderState->finalize(oldNode, false, matrix);
}

} // namespace Timeline
