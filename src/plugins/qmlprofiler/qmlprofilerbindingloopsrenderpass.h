// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerrangemodel.h"

#include <tracing/timelineabstractrenderer.h>
#include <tracing/timelinerenderpass.h>
#include <tracing/timelinerenderstate.h>

#include <QSGMaterial>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerBindingLoopsRenderPass : public Timeline::TimelineRenderPass
{
public:
    static const QmlProfilerBindingLoopsRenderPass *instance();
    State *update(const Timeline::TimelineAbstractRenderer *renderer,
                  const Timeline::TimelineRenderState *parentState,
                  State *oldState, int indexFrom, int indexTo, bool stateChanged,
                  float spacing) const override;
protected:
    QmlProfilerBindingLoopsRenderPass();
};

} // namespace Internal
} // namespace QmlProfiler
