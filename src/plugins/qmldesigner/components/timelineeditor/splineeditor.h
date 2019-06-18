/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
