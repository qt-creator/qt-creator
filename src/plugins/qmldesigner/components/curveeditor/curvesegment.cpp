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

#include "curvesegment.h"
#include "detail/utils.h"

#include <utils/qtcassert.h>
#include <QEasingCurve>
#include <QPainterPath>
#include <qmath.h>

#include <assert.h>

namespace DesignTools {

class CubicPolynomial
{
public:
    CubicPolynomial(double p0, double p1, double p2, double p3);

    std::vector<double> extrema() const;

    std::vector<double> roots() const;

private:
    double m_a;
    double m_b;
    double m_c;
    double m_d;
};

CubicPolynomial::CubicPolynomial(double p0, double p1, double p2, double p3)
    : m_a(p3 - 3.0 * p2 + 3.0 * p1 - p0)
    , m_b(3.0 * p2 - 6.0 * p1 + 3.0 * p0)
    , m_c(3.0 * p1 - 3.0 * p0)
    , m_d(p0)
{}

std::vector<double> CubicPolynomial::extrema() const
{
    std::vector<double> out;

    auto addValidValue = [&out](double value) {
        if (!std::isnan(value) && !std::isinf(value))
            out.push_back(clamp(value, 0.0, 1.0));
    };

    // Find the roots of the first derivative of y.
    auto pd2 = (2.0 * m_b) / (3.0 * m_a) / 2.0;
    auto q = m_c / (3.0 * m_a);

    auto radi = std::pow(pd2, 2.0) - q;

    auto x1 = -pd2 + std::sqrt(radi);
    auto x2 = -pd2 - std::sqrt(radi);

    addValidValue(x1);
    addValidValue(x2);

    return out;
}

std::vector<double> CubicPolynomial::roots() const
{
    std::vector<double> out;

    auto addValidValue = [&out](double value) {
        if (!(std::isnan(value) || std::isinf(value)))
            out.push_back(value);
    };

    if (m_a == 0.0) {
        // Linear
        if (m_b == 0.0) {
            if (m_c != 0.0)
                out.push_back(-m_d / m_c);
        // Quadratic
        } else {
            const double p = m_c / m_b / 2.0;
            const double q = m_d / m_b;
            addValidValue(-p + std::sqrt(std::pow(p, 2.0) - q));
            addValidValue(-p - std::sqrt(std::pow(p, 2.0) - q));
        }
    // Cubic
    } else {
        const double p = 3.0 * m_a * m_c - std::pow(m_b, 2.0);
        const double q = 2.0 * std::pow(m_b, 3.0) - 9.0 * m_a * m_b * m_c
                         + 27.0 * std::pow(m_a, 2.0) * m_d;

        auto disc = std::pow(q, 2.0) + 4.0 * std::pow(p, 3.0);

        auto toX = [&](double y) { return (y - m_b) / (3.0 * m_a); };

        // One real solution.
        if (disc >= 0) {
            auto u = (1.0 / 2.0)
                     * std::cbrt(-4.0 * q
                                 + 4.0 * std::sqrt(std::pow(q, 2.0) + 4.0 * std::pow(p, 3.0)));
            auto v = (1.0 / 2.0)
                     * std::cbrt(-4.0 * q
                                 - 4.0 * std::sqrt(std::pow(q, 2.0) + 4.0 * std::pow(p, 3.0)));

            addValidValue(toX(u + v));
        // Three real solutions.
        } else {
            auto phi = acos(-q / (2 * std::sqrt(-std::pow(p, 3.0))));
            auto y1 = std::sqrt(-p) * 2.0 * cos(phi / 3.0);
            auto y2 = std::sqrt(-p) * 2.0 * cos((phi / 3.0) + (2.0 * M_PI / 3.0));
            auto y3 = std::sqrt(-p) * 2.0 * cos((phi / 3.0) + (4.0 * M_PI / 3.0));

            addValidValue(toX(y1));
            addValidValue(toX(y2));
            addValidValue(toX(y3));
        }
    }
    return out;
}

CurveSegment::CurveSegment()
    : m_left()
    , m_right()
{}

CurveSegment::CurveSegment(const Keyframe &left, const Keyframe &right)
    : m_left(left)
    , m_right(right)
{}

bool CurveSegment::isValid() const
{
    if (m_left.position() == m_right.position())
        return false;

    if (interpolation() == Keyframe::Interpolation::Undefined)
        return false;

    if (interpolation() == Keyframe::Interpolation::Easing
        || interpolation() == Keyframe::Interpolation::Bezier) {
        if (qFuzzyCompare(m_left.position().y(), m_right.position().y()))
            return false;
    }
    return true;
}

bool CurveSegment::containsX(double x) const
{
    return m_left.position().x() <= x && m_right.position().x() >= x;
}

Keyframe CurveSegment::left() const
{
    return m_left;
}

Keyframe CurveSegment::right() const
{
    return m_right;
}

Keyframe::Interpolation CurveSegment::interpolation() const
{
    bool invalidBezier = m_right.interpolation() == Keyframe::Interpolation::Bezier
                         && (!m_left.hasRightHandle() || !m_right.hasLeftHandle());

    if (m_right.interpolation() == Keyframe::Interpolation::Undefined || invalidBezier)
        return Keyframe::Interpolation::Linear;

    return m_right.interpolation();
}

double evaluateForT(double t, double p0, double p1, double p2, double p3)
{
    QTC_ASSERT(t >= 0. && t <= 1., return 0.0);

    const double it = 1.0 - t;

    return p0 * std::pow(it, 3.0) + p1 * 3.0 * std::pow(it, 2.0) * t
           + p2 * 3.0 * it * std::pow(t, 2.0) + p3 * std::pow(t, 3.0);
}

QPointF CurveSegment::evaluate(double t) const
{
    if (interpolation() == Keyframe::Interpolation::Linear) {
        return lerp(t, m_left.position(), m_right.position());
    } else if (interpolation() == Keyframe::Interpolation::Step) {
        if (t == 1.0)
            return m_right.position();

        QPointF br(m_right.position().x(), m_left.position().y());
        return lerp(t, m_left.position(), br);
    } else if (interpolation() == Keyframe::Interpolation::Bezier) {
        const double x = evaluateForT(t,
                                      m_left.position().x(),
                                      m_left.rightHandle().x(),
                                      m_right.leftHandle().x(),
                                      m_right.position().x());

        const double y = evaluateForT(t,
                                      m_left.position().y(),
                                      m_left.rightHandle().y(),
                                      m_right.leftHandle().y(),
                                      m_right.position().y());

        return QPointF(x, y);
    }
    return QPointF();
}

QPainterPath CurveSegment::path() const
{
    QPainterPath path(m_left.position());
    extend(path);
    return path;
}

void CurveSegment::extendWithEasingCurve(QPainterPath &path, const QEasingCurve &curve) const
{
    auto mapEasing = [](const QPointF &start, const QPointF &end, const QPointF &pos) {
        QPointF slope(end.x() - start.x(), end.y() - start.y());
        return QPointF(start.x() + slope.x() * pos.x(), start.y() + slope.y() * pos.y());
    };

    QVector<QPointF> points = curve.toCubicSpline();
    int numSegments = points.count() / 3;
    for (int i = 0; i < numSegments; i++) {
        QPointF p1 = mapEasing(m_left.position(), m_right.position(), points.at(i * 3));
        QPointF p2 = mapEasing(m_left.position(), m_right.position(), points.at(i * 3 + 1));
        QPointF p3 = mapEasing(m_left.position(), m_right.position(), points.at(i * 3 + 2));
        path.cubicTo(p1, p2, p3);
    }
}

void CurveSegment::extend(QPainterPath &path) const
{
    if (interpolation() == Keyframe::Interpolation::Linear) {
        path.lineTo(m_right.position());
    } else if (interpolation() == Keyframe::Interpolation::Step) {
        path.lineTo(QPointF(m_right.position().x(), m_left.position().y()));
        path.lineTo(m_right.position());
    } else if (interpolation() == Keyframe::Interpolation::Bezier) {
        extendWithEasingCurve(path, easingCurve());
    } else if (interpolation() == Keyframe::Interpolation::Easing) {
        QVariant data = m_right.data();
        if (data.isValid() && data.type() == static_cast<int>(QMetaType::QEasingCurve)) {
            extendWithEasingCurve(path, data.value<QEasingCurve>());
        }
    }
}

QEasingCurve CurveSegment::easingCurve() const
{
    auto mapPosition = [this](const QPointF &position) {
        QPointF min = m_left.position();
        QPointF max = m_right.position();
        if (qFuzzyCompare(min.y(), max.y()))
            return QPointF((position.x() - min.x()) / (max.x() - min.x()),
                           (position.y() - min.y()) / (max.y()));

        return QPointF((position.x() - min.x()) / (max.x() - min.x()),
                       (position.y() - min.y()) / (max.y() - min.y()));
    };

    QEasingCurve curve;
    curve.addCubicBezierSegment(mapPosition(m_left.rightHandle()),
                                mapPosition(m_right.leftHandle()),
                                QPointF(1., 1.));
    return curve;
}

std::vector<QPointF> CurveSegment::extrema() const
{
    std::vector<QPointF> out;

    if (interpolation() == Keyframe::Interpolation::Linear
        || interpolation() == Keyframe::Interpolation::Step) {
        out.push_back(left().position());
        out.push_back(right().position());

    } else if (interpolation() == Keyframe::Interpolation::Bezier) {
        auto polynomial = CubicPolynomial(m_left.position().y(),
                                          m_left.rightHandle().y(),
                                          m_right.leftHandle().y(),
                                          m_right.position().y());

        for (double t : polynomial.extrema()) {
            const double x = evaluateForT(t,
                                          m_left.position().x(),
                                          m_left.rightHandle().x(),
                                          m_right.leftHandle().x(),
                                          m_right.position().x());

            const double y = evaluateForT(t,
                                          m_left.position().y(),
                                          m_left.rightHandle().y(),
                                          m_right.leftHandle().y(),
                                          m_right.position().y());

            out.push_back(QPointF(x, y));
        }
    }
    return out;
}

std::vector<double> CurveSegment::tForX(double x) const
{
    if (interpolation() == Keyframe::Interpolation::Linear) {
        return {reverseLerp(x, m_right.position().x(), m_left.position().x())};
    } else if (interpolation() == Keyframe::Interpolation::Step) {
        return {reverseLerp(x, m_left.position().x(), m_right.position().x())};
    } else if (interpolation() == Keyframe::Interpolation::Bezier) {
        auto polynomial = CubicPolynomial(m_left.position().x() - x,
                                          m_left.rightHandle().x() - x,
                                          m_right.leftHandle().x() - x,
                                          m_right.position().x() - x);

        std::vector<double> out;
        for (double t : polynomial.roots()) {
            if (t >= 0.0 && t <= 1.0)
                out.push_back(t);
        }
        return out;
    }

    return {};
}

std::vector<double> CurveSegment::tForY(double y) const
{
    auto polynomial = CubicPolynomial(m_left.position().y() - y,
                                      m_left.rightHandle().y() - y,
                                      m_right.leftHandle().y() - y,
                                      m_right.position().y() - y);

    std::vector<double> out;
    for (double t : polynomial.roots()) {
        if (t >= 0.0 && t <= 1.0)
            out.push_back(t);
    }
    return out;
}

std::vector<double> CurveSegment::yForX(double x) const
{
    std::vector<double> out;

    auto polynomial = CubicPolynomial(m_left.position().x() - x,
                                      m_left.rightHandle().x() - x,
                                      m_right.leftHandle().x() - x,
                                      m_right.position().x() - x);

    for (double t : polynomial.roots()) {
        if (t < 0.0 || t > 1.0)
            continue;

        const double y = evaluateForT(t,
                                      m_left.position().y(),
                                      m_left.rightHandle().y(),
                                      m_right.leftHandle().y(),
                                      m_right.position().y());

        out.push_back(y);
    }

    return out;
}

std::vector<double> CurveSegment::xForY(double y) const
{
    std::vector<double> out;

    auto polynomial = CubicPolynomial(m_left.position().y() - y,
                                      m_left.rightHandle().y() - y,
                                      m_right.leftHandle().y() - y,
                                      m_right.position().y() - y);

    for (double t : polynomial.roots()) {
        if (t < 0.0 || t > 1.0)
            continue;

        const double x = evaluateForT(t,
                                      m_left.position().x(),
                                      m_left.rightHandle().x(),
                                      m_right.leftHandle().x(),
                                      m_right.position().x());

        out.push_back(x);
    }

    return out;
}

std::array<Keyframe, 3> CurveSegment::splitAt(double time)
{
    std::array<Keyframe, 3> out;
    if (interpolation() == Keyframe::Interpolation::Linear) {
        for (double t : tForX(time)) {
            out[0] = left();
            out[1] = Keyframe(lerp(t, left().position(), right().position()));
            out[2] = right();

            out[1].setInterpolation(Keyframe::Interpolation::Linear);
            return out;
        }

    } else if (interpolation() == Keyframe::Interpolation::Step) {
        out[0] = left();
        out[1] = Keyframe(QPointF(left().position() + QPointF(time, 0.0)));
        out[2] = right();

        out[1].setInterpolation(Keyframe::Interpolation::Step);

    } else if (interpolation() == Keyframe::Interpolation::Bezier) {
        for (double t : tForX(time)) {
            auto p0 = lerp(t, left().position(), left().rightHandle());
            auto p1 = lerp(t, left().rightHandle(), right().leftHandle());
            auto p2 = lerp(t, right().leftHandle(), right().position());

            auto p01 = lerp(t, p0, p1);
            auto p12 = lerp(t, p1, p2);
            auto p01p12 = lerp(t, p01, p12);

            out[0] = Keyframe(left().position(), left().leftHandle(), p0);
            out[1] = Keyframe(p01p12, p01, p12);
            out[2] = Keyframe(right().position(), p2, right().rightHandle());

            out[0].setInterpolation(left().interpolation());
            out[0].setData(left().data());
            out[0].setUnified(left().isUnified());

            out[2].setInterpolation(right().interpolation());
            out[2].setData(right().data());
            out[2].setUnified(right().isUnified());
            return out;
        }
    }
    return out;
}

bool CurveSegment::intersects(const QPointF &coord, double radiusX, double radiusY) const
{
    if (interpolation() == Keyframe::Interpolation::Linear) {
        for (auto &t : tForX(coord.x())) {
            QLineF line(evaluate(t), coord);
            if (std::abs(line.dy()) < radiusY)
                return true;
        }
    } else if (interpolation() == Keyframe::Interpolation::Step) {
        if (coord.x() > (right().position().x() - radiusX))
            return true;

        if (coord.y() > (left().position().y() - radiusY)
            && coord.y() < (left().position().y() + radiusY))
            return true;

    } else if (interpolation() == Keyframe::Interpolation::Bezier) {
        for (auto &y : yForX(coord.x())) {
            QLineF line(coord.x(), y, coord.x(), coord.y());
            if (line.length() < radiusY)
                return true;
        }

        for (auto &x : xForY(coord.y())) {
            QLineF line(x, coord.y(), coord.x(), coord.y());
            if (line.length() < radiusX)
                return true;
        }
    }
    return false;
}

void CurveSegment::setLeft(const Keyframe &frame)
{
    m_left = frame;
}

void CurveSegment::setRight(const Keyframe &frame)
{
    m_right = frame;
}

void CurveSegment::setInterpolation(const Keyframe::Interpolation &interpol)
{
    m_right.setInterpolation(interpol);

    if (interpol == Keyframe::Interpolation::Bezier) {
        double distance = QLineF(m_left.position(), m_right.position()).length() / 3.0;
        if (!m_left.hasRightHandle())
            m_left.setRightHandle(m_left.position() + QPointF(distance, 0.0));

        if (!m_right.hasLeftHandle())
            m_right.setLeftHandle(m_right.position() - QPointF(distance, 0.0));

    } else {
        m_left.setRightHandle(QPointF());
        m_right.setLeftHandle(QPointF());
    }
}

} // End namespace DesignTools.
