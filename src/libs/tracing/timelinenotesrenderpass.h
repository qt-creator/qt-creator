// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstractrenderer.h"
#include <QSGMaterial>

namespace Timeline {

class TRACING_EXPORT TimelineNotesRenderPass : public TimelineRenderPass
{
public:
    static const TimelineNotesRenderPass *instance();

    State *update(const TimelineAbstractRenderer *renderer, const TimelineRenderState *parentState,
                  State *oldState, int firstIndex, int lastIndex, bool stateChanged,
                  float spacing) const override;

private:
    TimelineNotesRenderPass();
};

} // namespace Timeline
