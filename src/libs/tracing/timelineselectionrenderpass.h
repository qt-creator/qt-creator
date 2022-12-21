// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstractrenderer.h"
#include "timelinerenderpass.h"
#include "timelinerenderstate.h"

namespace Timeline {

class TRACING_EXPORT TimelineSelectionRenderPass : public TimelineRenderPass
{
public:
    static const TimelineSelectionRenderPass *instance();

    State *update(const TimelineAbstractRenderer *renderer, const TimelineRenderState *parentState,
                  State *state, int firstIndex, int lastIndex, bool stateChanged,
                  float spacing) const override;

protected:
    TimelineSelectionRenderPass();
};

} // namespace Timeline
