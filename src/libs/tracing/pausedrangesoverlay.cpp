// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pausedrangesoverlay.h"

#include "timelinecoordinates.h"
#include "timelinemodelaggregator.h"
#include "timelinezoomcontrol.h"
#include "tracingtr.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QPainter>

namespace Timeline {

PausedRangesOverlay::PausedRangesOverlay(TimelineModelAggregator *aggregator,
                                         TimelineZoomControl *zoom, QWidget *parent)
    : QWidget(parent)
    , m_aggregator(aggregator)
    , m_zoom(zoom)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAutoFillBackground(false);

    connect(aggregator, &TimelineModelAggregator::pausedRangesChanged,
            this, qOverload<>(&QWidget::update));
    connect(zoom, &TimelineZoomControl::rangeChanged, this, qOverload<>(&QWidget::update));
}

void PausedRangesOverlay::paintBand(QPainter &painter, const QRectF &band, BandEdges edges)
{
    using namespace Utils;
    QColor fillColor = creatorColor(Theme::Token_Background_Muted);
    fillColor.setAlphaF(0.6);
    QColor hatchColor = creatorColor(Theme::Token_Stroke_Subtle);
    hatchColor.setAlphaF(0.5);

    painter.fillRect(band, fillColor);
    painter.fillRect(band, QBrush(hatchColor, Qt::BDiagPattern));
    if (edges == BandEdges::Draw) {
        painter.setPen(hatchColor);
        painter.drawLine(QPointF(band.left(), band.top()), QPointF(band.left(), band.bottom()));
        painter.drawLine(QPointF(band.right(), band.top()), QPointF(band.right(), band.bottom()));
    }
}

void PausedRangesOverlay::paintEvent(QPaintEvent *)
{
    const auto &ranges = m_aggregator->pausedRanges();
    if (ranges.isEmpty())
        return;

    const QColor textColor = Utils::creatorColor(Utils::Theme::Token_Text_Muted);

    QPainter p(this);
    p.setFont(Utils::StyleHelper::uiFont(Utils::StyleHelper::UiElementCaptionStrong));
    const double w = width();
    const QString label = Tr::tr("Paused");
    const int labelWidth = p.fontMetrics().horizontalAdvance(label);
    const int labelInsetH = Utils::StyleHelper::SpacingTokens::PaddingHS;
    const int labelInsetV = Utils::StyleHelper::SpacingTokens::PaddingVXs;

    for (const auto &[start, end] : ranges) {
        const double x1 = timeToPixel(start, m_zoom->rangeStart(), m_zoom->rangeEnd(), w);
        const double x2 = timeToPixel(end, m_zoom->rangeStart(), m_zoom->rangeEnd(), w);
        if (x2 < 0 || x1 > w)
            continue;
        const QRectF band(qMax(0.0, x1), 0, qMax(1.0, qMin(w, x2) - qMax(0.0, x1)), height());
        paintBand(p, band, BandEdges::Draw);
        if (band.width() > labelWidth + 2 * labelInsetH) {
            p.setPen(textColor);
            p.drawText(band.adjusted(labelInsetH, labelInsetV, -labelInsetH, 0),
                       Qt::AlignLeft | Qt::AlignTop, label);
        }
    }
}

} // namespace Timeline
