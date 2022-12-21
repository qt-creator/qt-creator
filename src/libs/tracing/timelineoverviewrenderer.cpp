// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineoverviewrenderer_p.h"
#include "timelinerenderstate.h"

namespace Timeline {

TimelineOverviewRenderer::TimelineOverviewRenderer(QQuickItem *parent) :
    TimelineAbstractRenderer(*(new TimelineOverviewRendererPrivate), parent)
{
}

TimelineOverviewRenderer::TimelineOverviewRendererPrivate::TimelineOverviewRendererPrivate() :
    renderState(nullptr)
{
}

TimelineOverviewRenderer::TimelineOverviewRendererPrivate::~TimelineOverviewRendererPrivate()
{
    delete renderState;
}

QSGNode *TimelineOverviewRenderer::updatePaintNode(QSGNode *oldNode,
                                                  UpdatePaintNodeData *updatePaintNodeData)
{
    Q_D(TimelineOverviewRenderer);

    if (!d->model || d->model->isEmpty() || !d->zoomer || d->zoomer->traceDuration() <= 0) {
        delete oldNode;
        return nullptr;
    }

    if (d->modelDirty) {
        delete d->renderState;
        d->renderState = nullptr;
    }

    if (d->renderState == nullptr) {
        d->renderState = new TimelineRenderState(d->zoomer->traceStart(), d->zoomer->traceEnd(),
                                                 1.0, d->renderPasses.size());
    }

    float xSpacing = static_cast<float>(width() / d->zoomer->traceDuration());
    float ySpacing = static_cast<float>(
                height() / (d->model->collapsedRowCount() * TimelineModel::defaultRowHeight()));

    for (int i = 0; i < d->renderPasses.length(); ++i) {
        d->renderState->setPassState(i, d->renderPasses[i]->update(this, d->renderState,
                                                                   d->renderState->passState(i),
                                                                   0, d->model->count(), true,
                                                                   xSpacing));
    }

    if (d->renderState->isEmpty())
        d->renderState->assembleNodeTree(d->model, d->model->height(), 0);

    TimelineAbstractRenderer::updatePaintNode(nullptr, updatePaintNodeData);

    QMatrix4x4 matrix;
    matrix.scale(xSpacing, ySpacing, 1);
    return d->renderState->finalize(oldNode, false, matrix);
}

}

