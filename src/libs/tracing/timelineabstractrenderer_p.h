// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstractrenderer.h"

#include <QPointer>

namespace Timeline {

class TRACING_EXPORT TimelineAbstractRenderer::TimelineAbstractRendererPrivate {
public:
    TimelineAbstractRendererPrivate();
    virtual ~TimelineAbstractRendererPrivate();

    int selectedItem;
    bool selectionLocked;
    QPointer<TimelineModel> model;
    QPointer<TimelineNotesModel> notes;
    QPointer<TimelineZoomControl> zoomer;

    bool modelDirty;
    bool rowHeightsDirty;
    bool notesDirty;

    QList<const TimelineRenderPass *> renderPasses;
};

}
