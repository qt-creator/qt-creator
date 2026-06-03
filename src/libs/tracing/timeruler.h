// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QList>
#include <QWidget>

namespace Timeline {

class TRACING_EXPORT TimeRuler : public QWidget
{
    Q_OBJECT
public:
    explicit TimeRuler(QWidget *parent = nullptr);

    void setRange(qint64 rangeStart, qint64 rangeEnd);

    qint64 rangeStart() const { return m_rangeStart; }
    qint64 rangeEnd() const { return m_rangeEnd; }

    QSize sizeHint() const override;

signals:
    void markersChanged(const QList<qint64> &markers);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    int markerAt(int x) const;
    double markerPixel(qint64 timestamp) const;

    qint64 m_rangeStart = 0;
    qint64 m_rangeEnd = 0;
    QList<qint64> m_markers;

    int m_dragIndex = -1;
    int m_dragStartX = 0;
    qint64 m_dragStartTime = 0;
    bool m_dragged = false;
};

} // namespace Timeline
