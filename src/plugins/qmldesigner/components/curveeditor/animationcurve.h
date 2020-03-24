/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "keyframe.h"

#include <array>
#include <vector>

QT_FORWARD_DECLARE_CLASS(QEasingCurve);
QT_FORWARD_DECLARE_CLASS(QPainterPath);

namespace DesignTools {

class CurveSegment;

class AnimationCurve
{
public:
    AnimationCurve();

    AnimationCurve(const std::vector<Keyframe> &frames);

    AnimationCurve(const QEasingCurve &easing, const QPointF &start, const QPointF &end);

    bool isValid() const;

    bool isFromData() const;

    bool hasUnified() const;

    double minimumTime() const;

    double maximumTime() const;

    double minimumValue() const;

    double maximumValue() const;

    std::string string() const;

    QString unifyString() const;

    CurveSegment segment(double time) const;

    std::vector<CurveSegment> segments() const;

    QPainterPath simplePath() const;

    QPainterPath intersectionPath() const;

    Keyframe keyframeAt(size_t id) const;

    std::vector<Keyframe> keyframes() const;

    std::vector<QPointF> extrema() const;

    std::vector<double> yForX(double x) const;

    std::vector<double> xForY(double y, uint segment) const;

    bool intersects(const QPointF &coord, double radiusX, double radiusY) const;

    void append(const AnimationCurve &other);

    void insert(double time);

private:
    void analyze();

    bool m_fromData;

    double m_minY;

    double m_maxY;

    std::vector<Keyframe> m_frames;
};

} // End namespace DesignTools.
