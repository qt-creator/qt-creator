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

QT_BEGIN_NAMESPACE
class QPointF;
class QEasingCurve;
class QPainterPath;
QT_END_NAMESPACE

namespace DesignTools {

class CurveSegment
{
public:
    CurveSegment();

    CurveSegment(const Keyframe &first, const Keyframe &last);

    bool isValid() const;

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

    void setInterpolation(const Keyframe::Interpolation &interpol);

private:
    Keyframe m_left;

    Keyframe m_right;
};

} // End namespace DesignTools.
