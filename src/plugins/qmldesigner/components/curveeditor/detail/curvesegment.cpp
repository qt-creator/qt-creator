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
#include "utils.h"

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

bool CurveSegment::containsX(double x) const
{
    return m_left.position().x() <= x && m_right.position().x() >= x;
}

double evaluateForT(double t, double p0, double p1, double p2, double p3)
{
    assert(t >= 0. && t <= 1.);

    const double it = 1.0 - t;

    return p0 * std::pow(it, 3.0) + p1 * 3.0 * std::pow(it, 2.0) * t
           + p2 * 3.0 * it * std::pow(t, 2.0) + p3 * std::pow(t, 3.0);
}

QPointF CurveSegment::evaluate(double t) const
{
    const double x = evaluateForT(
        t,
        m_left.position().x(),
        m_left.rightHandle().x(),
        m_right.leftHandle().x(),
        m_right.position().x());

    const double y = evaluateForT(
        t,
        m_left.position().y(),
        m_left.rightHandle().y(),
        m_right.leftHandle().y(),
        m_right.position().y());

    return QPointF(x, y);
}

std::vector<QPointF> CurveSegment::extrema() const
{
    std::vector<QPointF> out;

    auto polynomial = CubicPolynomial(
        m_left.position().y(),
        m_left.rightHandle().y(),
        m_right.leftHandle().y(),
        m_right.position().y());

    for (double t : polynomial.extrema()) {

        const double x = evaluateForT(
            t,
            m_left.position().x(),
            m_left.rightHandle().x(),
            m_right.leftHandle().x(),
            m_right.position().x());

        const double y = evaluateForT(
            t,
            m_left.position().y(),
            m_left.rightHandle().y(),
            m_right.leftHandle().y(),
            m_right.position().y());

        out.push_back(QPointF(x, y));
    }
    return out;
}

std::vector<double> CurveSegment::yForX(double x) const
{
    std::vector<double> out;

    auto polynomial = CubicPolynomial(
        m_left.position().x() - x,
        m_left.rightHandle().x() - x,
        m_right.leftHandle().x() - x,
        m_right.position().x() - x);

    for (double t : polynomial.roots()) {
        if (t < 0.0 || t > 1.0)
            continue;

        const double y = evaluateForT(
            t,
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

    auto polynomial = CubicPolynomial(
        m_left.position().y() - y,
        m_left.rightHandle().y() - y,
        m_right.leftHandle().y() - y,
        m_right.position().y() - y);

    for (double t : polynomial.roots()) {
        if (t < 0.0 || t > 1.0)
            continue;

        const double x = evaluateForT(
            t,
            m_left.position().x(),
            m_left.rightHandle().x(),
            m_right.leftHandle().x(),
            m_right.position().x());

        out.push_back(x);
    }

    return out;
}

void CurveSegment::setLeft(const Keyframe &frame)
{
    m_left = frame;
}

void CurveSegment::setRight(const Keyframe &frame)
{
    m_right = frame;
}

} // End namespace DesignTools.
