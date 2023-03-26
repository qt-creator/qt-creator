// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

#include "canvas.h"
#include "easingcurve.h"

QT_FORWARD_DECLARE_CLASS(QPropertyAnimation)

namespace QmlDesigner {

class SegmentProperties;

class SplineEditor : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(double progress READ progress WRITE setProgress)

signals:
    void easingCurveChanged(const EasingCurve &curve);

public:
    explicit SplineEditor(QWidget *parent = nullptr);

    double progress() const;

    EasingCurve easingCurve() const;

    void animate() const;

    void setDuration(int duration);

    void setProgress(double progress);

    void setEasingCurve(const EasingCurve &curve);

protected:
    void resizeEvent(QResizeEvent *) override;

    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *) override;

    void mouseMoveEvent(QMouseEvent *) override;

    void mouseReleaseEvent(QMouseEvent *) override;

    void contextMenuEvent(QContextMenuEvent *) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

private:
    Canvas m_canvas;

    EasingCurve m_curve;

    QPoint m_mousePress;

    bool m_mouseDrag = false;

    bool m_block = false;

    double m_progress = 0;

    QPropertyAnimation *m_animation;
};

} // namespace QmlDesigner
