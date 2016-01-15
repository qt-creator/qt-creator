/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "timelineoverviewrenderer_p.h"
#include "timelinerenderstate.h"

namespace Timeline {

TimelineOverviewRenderer::TimelineOverviewRenderer(QQuickItem *parent) :
    TimelineAbstractRenderer(*(new TimelineOverviewRendererPrivate), parent)
{
}

TimelineOverviewRenderer::TimelineOverviewRendererPrivate::TimelineOverviewRendererPrivate() :
    renderState(0)
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
        return 0;
    }

    if (d->modelDirty) {
        delete d->renderState;
        d->renderState = 0;
    }

    if (d->renderState == 0) {
        d->renderState = new TimelineRenderState(d->zoomer->traceStart(), d->zoomer->traceEnd(),
                                                 1.0, d->renderPasses.size());
    }

    qreal xSpacing = width() / d->zoomer->traceDuration();
    qreal ySpacing = height() / (d->model->collapsedRowCount() * TimelineModel::defaultRowHeight());

    for (int i = 0; i < d->renderPasses.length(); ++i) {
        d->renderState->setPassState(i, d->renderPasses[i]->update(this, d->renderState,
                                                                   d->renderState->passState(i),
                                                                   0, d->model->count(), true,
                                                                   xSpacing));
    }

    if (d->renderState->isEmpty())
        d->renderState->assembleNodeTree(d->model, d->model->height(), 0);

    TimelineAbstractRenderer::updatePaintNode(0, updatePaintNodeData);

    QMatrix4x4 matrix;
    matrix.scale(xSpacing, ySpacing, 1);
    return d->renderState->finalize(oldNode, false, matrix);
}

}

