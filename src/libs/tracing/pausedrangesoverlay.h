// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPainter;
class QRectF;
QT_END_NAMESPACE

namespace Timeline {

class TimelineModelAggregator;
class TimelineZoomControl;

// Covers the tracks with a hatched band wherever the recording was paused.
// Passes every mouse event through to what is underneath.
class TRACING_EXPORT PausedRangesOverlay : public QWidget
{
    Q_OBJECT
public:
    PausedRangesOverlay(TimelineModelAggregator *aggregator, TimelineZoomControl *zoom,
                        QWidget *parent = nullptr);

    enum class BandEdges : quint8 { None, Draw };

    // Fills `band` the way a paused stretch is shown, here and on the overview.
    // The edges are the lines along its sides.
    static void paintBand(QPainter &painter, const QRectF &band, BandEdges edges);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    TimelineModelAggregator *m_aggregator;
    TimelineZoomControl *m_zoom;
};

} // namespace Timeline
