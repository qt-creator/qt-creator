// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Timeline {

class TRACING_EXPORT SelectionRangeDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectionRangeDetailsWidget(QWidget *parent = nullptr);

    void setData(qint64 start, qint64 end, qint64 referenceDuration, bool showDuration);

signals:
    void closeRequested();
    void recenterRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_startValue;
    QLabel *m_endKey;
    QLabel *m_endValue;
    QLabel *m_durationKey;
    QLabel *m_durationValue;

    QPoint m_dragOffset;
    bool m_dragging = false;
    bool m_didDrag = false;
};

} // namespace Timeline
