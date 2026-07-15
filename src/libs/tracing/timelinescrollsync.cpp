// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinescrollsync.h"

#include "timeruler.h"
#include "tracklabels.h"
#include "trackpainterbase.h"
#include "timelinezoomcontrol.h"

#include <utils/qtcassert.h>

#include <QEvent>
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

    if (!m_ruler && m_scrollBar) {
        m_ruler = ruler;
        m_scrollBar->installEventFilter(this);
        updateRulerMargins();
    }
}

void TimelineScrollSync::registerContent(TrackPainterBase *painter)
{
    painter->setRange(m_zoom->rangeStart(), m_zoom->rangeEnd());
    // TrackPainterBase is not a QObject; use its widget as the connection
    // context so the lambda is disconnected when the track area is destroyed.
    connect(m_zoom, &TimelineZoomControl::rangeChanged, painter->widget(),
            [painter](qint64 start, qint64 end) { painter->setRange(start, end); });
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
    for (auto labels : std::as_const(m_labels))
        connect(scrollBar, &QScrollBar::valueChanged, labels, &TrackLabels::setScrollOffset);

    if (m_ruler) {
        scrollBar->installEventFilter(this);
        updateRulerMargins();
    }
}

bool TimelineScrollSync::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_scrollBar
        && (event->type() == QEvent::Show || event->type() == QEvent::Hide)) {
        updateRulerMargins();
    }
    return QObject::eventFilter(watched, event);
}

void TimelineScrollSync::updateRulerMargins()
{
    if (!m_ruler)
        return;

    const int scrollBarWidth = (m_scrollBar && m_scrollBar->isVisible())
                                   ? m_scrollBar->sizeHint().width() : 0;
    m_ruler->setContentsMargins(0, 0, scrollBarWidth, 0);
}

} // namespace Timeline
