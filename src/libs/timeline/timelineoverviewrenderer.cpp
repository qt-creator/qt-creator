/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

