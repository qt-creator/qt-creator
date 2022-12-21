// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineoverviewrenderer.h"
#include "timelineabstractrenderer_p.h"

namespace Timeline {

class TimelineOverviewRenderer::TimelineOverviewRendererPrivate :
        public TimelineAbstractRenderer::TimelineAbstractRendererPrivate
{
public:
    TimelineOverviewRendererPrivate();
    ~TimelineOverviewRendererPrivate();

    TimelineRenderState *renderState;
};

} // namespace Timeline
