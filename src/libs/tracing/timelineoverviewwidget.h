// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QPixmap>
#include <QWidget>

namespace Timeline {

class TimelineModelAggregator;
class TimelineZoomControl;

class TRACING_EXPORT TimelineOverviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineOverviewWidget(TimelineModelAggregator *aggregator,
                                    TimelineZoomControl *zoom,
                                    QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    double timeToPixel(qint64 t) const;
    qint64 pixelToTime(double px) const;
    bool nearHandle(double px, double handlePx) const;
    void applyDrag(double px);
    void rebuildContentCache();

    enum DragMode { DragNone, DragLeft, DragRight, DragRange };

    TimelineModelAggregator *m_aggregator;
    TimelineZoomControl *m_zoom;

    DragMode m_dragMode = DragNone;
    double m_dragStartX = 0;
    qint64 m_dragRangeStart = 0;
    qint64 m_dragRangeEnd = 0;

    bool m_leftHovered = false;
    bool m_rightHovered = false;

    QPixmap m_contentCache;
    bool m_contentDirty = true;
};

} // namespace Timeline
