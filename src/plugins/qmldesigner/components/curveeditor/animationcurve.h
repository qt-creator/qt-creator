// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "keyframe.h"

#include <array>
#include <vector>

QT_FORWARD_DECLARE_CLASS(QEasingCurve);
QT_FORWARD_DECLARE_CLASS(QPainterPath);

namespace QmlDesigner {

class CurveSegment;

class AnimationCurve
{
public:
    using ValueType = Keyframe::ValueType;

    AnimationCurve();

    AnimationCurve(ValueType type, const std::vector<Keyframe> &frames);

    AnimationCurve(
        ValueType type,
        const QEasingCurve &easing,
        const QPointF &start,
        const QPointF &end);

    bool isEmpty() const;

    bool isValid() const;

    bool isFromData() const;

    bool hasUnified() const;

    ValueType valueType() const;

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

    ValueType m_type;

    bool m_fromData;

    double m_minY;

    double m_maxY;

    std::vector<Keyframe> m_frames;
};

} // End namespace QmlDesigner.
