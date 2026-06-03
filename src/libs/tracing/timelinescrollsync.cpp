// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinescrollsync.h"

#include "timeruler.h"
#include "tracklabels.h"
#include "trackpainter.h"
#include "timelinezoomcontrol.h"

#include <QScrollBar>

namespace Timeline {

TimelineScrollSync::TimelineScrollSync(TimelineZoomControl *zoom, QObject *parent)
    : QObject(parent)
    , m_zoom(zoom)
{
}

void TimelineScrollSync::registerRuler(TimeRuler *ruler)
{
    ruler->setRange(m_zoom->rangeStart(), m_zoom->rangeEnd());
    connect(m_zoom, &TimelineZoomControl::rangeChanged, ruler, &TimeRuler::setRange);
}

void TimelineScrollSync::registerContent(TrackPainter *painter)
{
    painter->setRange(m_zoom->rangeStart(), m_zoom->rangeEnd());
    connect(m_zoom, &TimelineZoomControl::rangeChanged, painter, &TrackPainter::setRange);
}

void TimelineScrollSync::registerLabels(TrackLabels *labels)
{
    m_labels.append(labels);
    connect(labels, &QObject::destroyed, this, [this, labels] {
        m_labels.removeOne(labels);
    });
    if (m_scrollBar)
        connect(m_scrollBar, &QScrollBar::valueChanged, labels, &TrackLabels::setScrollOffset);
}

void TimelineScrollSync::setVerticalScrollBar(QScrollBar *scrollBar)
{
    m_scrollBar = scrollBar;
    for (auto labels : m_labels)
        connect(scrollBar, &QScrollBar::valueChanged, labels, &TrackLabels::setScrollOffset);
}

} // namespace Timeline
