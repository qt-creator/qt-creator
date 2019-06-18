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

#include "animationcurve.h"
#include "detail/curvesegment.h"

#include <QLineF>

namespace DesignTools {

AnimationCurve::AnimationCurve()
    : m_frames()
{}

AnimationCurve::AnimationCurve(const std::vector<Keyframe> &frames)
    : m_frames(frames)
    , m_minY(std::numeric_limits<double>::max())
    , m_maxY(std::numeric_limits<double>::lowest())
{
    if (isValid()) {

        for (auto e : extrema()) {

            if (m_minY > e.y())
                m_minY = e.y();

            if (m_maxY < e.y())
                m_maxY = e.y();
        }

        for (auto &frame : qAsConst(m_frames)) {
            if (frame.position().y() < m_minY)
                m_minY = frame.position().y();

            if (frame.position().y() > m_maxY)
                m_maxY = frame.position().y();
        }
    }
}

bool AnimationCurve::isValid() const
{
    return m_frames.size() >= 2;
}

double AnimationCurve::minimumTime() const
{
    if (!m_frames.empty())
        return m_frames.front().position().x();

    return std::numeric_limits<double>::max();
}

double AnimationCurve::maximumTime() const
{
    if (!m_frames.empty())
        return m_frames.back().position().x();

    return std::numeric_limits<double>::lowest();
}

double AnimationCurve::minimumValue() const
{
    return m_minY;
}

double AnimationCurve::maximumValue() const
{
    return m_maxY;
}

std::vector<Keyframe> AnimationCurve::keyframes() const
{
    return m_frames;
}

std::vector<QPointF> AnimationCurve::extrema() const
{
    std::vector<QPointF> out;

    CurveSegment segment;
    segment.setLeft(m_frames.at(0));

    for (size_t i = 1; i < m_frames.size(); ++i) {

        segment.setRight(m_frames[i]);

        const auto es = segment.extrema();
        out.insert(std::end(out), std::begin(es), std::end(es));

        segment.setLeft(m_frames[i]);
    }

    return out;
}

std::vector<double> AnimationCurve::yForX(double x) const
{
    if (m_frames.front().position().x() > x)
        return std::vector<double>();

    CurveSegment segment;
    for (auto &frame : m_frames) {
        if (frame.position().x() > x) {
            segment.setRight(frame);
            return segment.yForX(x);
        }
        segment.setLeft(frame);
    }
    return std::vector<double>();
}

std::vector<double> AnimationCurve::xForY(double y, uint segment) const
{
    if (m_frames.size() > segment + 1) {
        CurveSegment seg(m_frames[segment], m_frames[segment + 1]);
        return seg.xForY(y);
    }
    return std::vector<double>();
}

bool AnimationCurve::intersects(const QPointF &coord, double radius)
{
    if (m_frames.size() < 2)
        return false;

    std::vector<CurveSegment> influencer;

    CurveSegment current;
    current.setLeft(m_frames.at(0));

    for (size_t i = 1; i < m_frames.size(); ++i) {
        Keyframe &frame = m_frames.at(i);

        current.setRight(frame);

        if (current.containsX(coord.x() - radius) ||
            current.containsX(coord.x()) ||
            current.containsX(coord.x() + radius)) {
            influencer.push_back(current);
        }

        if (frame.position().x() > coord.x() + radius)
            break;

        current.setLeft(frame);
    }

    for (auto &segment : influencer) {
        for (auto &y : segment.yForX(coord.x())) {
            QLineF line(coord.x(), y, coord.x(), coord.y());
            if (line.length() < radius)
                return true;
        }

        for (auto &x : segment.xForY(coord.y())) {
            QLineF line(x, coord.y(), coord.x(), coord.y());
            if (line.length() < radius)
                return true;
        }
    }
    return false;
}

} // End namespace DesignTools.
