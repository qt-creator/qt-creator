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
