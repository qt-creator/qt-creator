// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinerenderstate.h"

namespace Timeline {

class TimelineRenderState::TimelineRenderStatePrivate {
public:
    QSGNode *expandedRowRoot;
    QSGNode *collapsedRowRoot;
    QSGNode *expandedOverlayRoot;
    QSGNode *collapsedOverlayRoot;

    qint64 start;
    qint64 end;

    float scale;  // "native" scale, this stays the same through the life time of a state

    QVector<TimelineRenderPass::State *> passes;
};

} // namespace Timeline
