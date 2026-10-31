// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QList>
#include <QWidget>

#include <chrono>

namespace Timeline {

class TRACING_EXPORT TimeRuler : public QWidget
{
    Q_OBJECT
public:
    explicit TimeRuler(QWidget *parent = nullptr);

    void setRange(std::chrono::nanoseconds rangeStart, std::chrono::nanoseconds rangeEnd);

    std::chrono::nanoseconds rangeStart() const { return m_rangeStart; }
    std::chrono::nanoseconds rangeEnd() const { return m_rangeEnd; }

    QSize sizeHint() const override;

signals:
    void markersChanged(const QList<std::chrono::nanoseconds> &markers);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    int markerAt(int x) const;
    double markerPixel(std::chrono::nanoseconds timestamp) const;

    std::chrono::nanoseconds m_rangeStart{0};
    std::chrono::nanoseconds m_rangeEnd{0};
    QList<std::chrono::nanoseconds> m_markers;

    int m_dragIndex = -1;
    int m_dragStartX = 0;
    std::chrono::nanoseconds m_dragStartTime{0};
    bool m_dragged = false;
};

} // namespace Timeline
