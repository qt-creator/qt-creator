// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <tracing/timelinerenderpass.h>

namespace PerfProfiler {
namespace Internal {

class PerfTimelineResourcesRenderPass : public Timeline::TimelineRenderPass
{
public:
    static const PerfTimelineResourcesRenderPass *instance();

    Timeline::TimelineRenderPass::State *update(
            const Timeline::TimelineAbstractRenderer *renderer,
            const Timeline::TimelineRenderState *parentState,
            Timeline::TimelineRenderPass::State *oldState, int indexFrom, int indexTo,
            bool stateChanged, float spacing) const override;

private:
    PerfTimelineResourcesRenderPass() = default;
};

} // namespace Internal
} // namespace PerfProfiler

