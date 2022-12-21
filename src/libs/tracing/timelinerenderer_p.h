// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinerenderer.h"
#include "timelineabstractrenderer_p.h"

namespace Timeline {

class TimelineRenderer::TimelineRendererPrivate :
        public TimelineAbstractRenderer::TimelineAbstractRendererPrivate {
public:
    enum MatchResult {
        NoMatch,
        Cutoff,
        ApproximateMatch,
        ExactMatch
    };

    struct MatchParameters {
        qint64 startTime;
        qint64 endTime;
        qint64 exactTime;
        qint64 bestOffset;
    };

    TimelineRendererPrivate();
    ~TimelineRendererPrivate();

    void clear();

    int rowFromPosition(int y) const;

    MatchResult checkMatch(MatchParameters *params, int index, qint64 itemStart, qint64 itemEnd);
    MatchResult matchForward(MatchParameters *params, int index);
    MatchResult matchBackward(MatchParameters *params, int index);

    void findCurrentSelection(int mouseX, int mouseY, int width);

    static const int SafeFloatMax = 1 << 12;

    void resetCurrentSelection();

    TimelineRenderState *findRenderState();

    int currentEventIndex;
    int currentRow;

    QVector<QHash<qint64, TimelineRenderState *> > renderStates;
    TimelineRenderState *lastState;
};

} // namespace Timeline
