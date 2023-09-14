// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "keyframe.h"

#include <array>
#include <vector>

QT_BEGIN_NAMESPACE
class QPointF;
class QEasingCurve;
class QPainterPath;
QT_END_NAMESPACE

namespace QmlDesigner {

class CurveSegment
{
public:
    CurveSegment();

    CurveSegment(const Keyframe &first, const Keyframe &last);

    bool isValid() const;

    bool isLegal() const;

    bool isLegalMcu() const;

    bool containsX(double x) const;

    Keyframe left() const;

    Keyframe right() const;

    Keyframe::Interpolation interpolation() const;

    QPointF evaluate(double t) const;

    QPainterPath path() const;

    void extend(QPainterPath &path) const;

    void extendWithEasingCurve(QPainterPath &path, const QEasingCurve &curve) const;

    QEasingCurve easingCurve() const;

    std::vector<QPointF> extrema() const;

    std::vector<double> tForX(double x) const;

    std::vector<double> tForY(double y) const;

    std::vector<double> yForX(double x) const;

    std::vector<double> xForY(double y) const;

    std::array<Keyframe, 3> splitAt(double time);

    bool intersects(const QPointF &coord, double radiusX, double radiusY) const;

    void setLeft(const Keyframe &frame);

    void setRight(const Keyframe &frame);

    void moveLeftTo(const QPointF &pos);

    void moveRightTo(const QPointF &pos);

    void setInterpolation(const Keyframe::Interpolation &interpol);

private:
    Keyframe m_left;

    Keyframe m_right;
};

} // End namespace QmlDesigner.
