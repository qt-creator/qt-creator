// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "animationcurve.h"
#include "curvesegment.h"
#include "detail/curveeditorutils.h"

#include <QEasingCurve>
#include <QLineF>
#include <QPainterPath>

#include <sstream>

namespace QmlDesigner {

AnimationCurve::AnimationCurve()
    : m_type(AnimationCurve::ValueType::Undefined)
    , m_fromData(false)
    , m_minY(std::numeric_limits<double>::max())
    , m_maxY(std::numeric_limits<double>::lowest())
    , m_frames()
{}

AnimationCurve::AnimationCurve(AnimationCurve::ValueType type, const std::vector<Keyframe> &frames)
    : m_type(type)
    , m_fromData(false)
    , m_minY(std::numeric_limits<double>::max())
    , m_maxY(std::numeric_limits<double>::lowest())
    , m_frames(frames)
{
    analyze();
}

AnimationCurve::AnimationCurve(
    AnimationCurve::ValueType type,
    const QEasingCurve &easing,
    const QPointF &start,
    const QPointF &end)
    : m_type(type)
    , m_fromData(true)
    , m_minY(std::numeric_limits<double>::max())
    , m_maxY(std::numeric_limits<double>::lowest())
    , m_frames()
{
    auto mapPosition = [start, end](const QPointF &pos) {
        QPointF slope(end.x() - start.x(), end.y() - start.y());
        return QPointF(start.x() + slope.x() * pos.x(), start.y() + slope.y() * pos.y());
    };

    QList<QPointF> points = easing.toCubicSpline();
    int numSegments = points.size() / 3;

    Keyframe current;
    Keyframe tmp(start);

    current.setInterpolation(Keyframe::Interpolation::Linear);
    tmp.setInterpolation(Keyframe::Interpolation::Bezier);

    for (int i = 0; i < numSegments; i++) {
        QPointF p1 = mapPosition(points.at(i * 3));
        QPointF p2 = mapPosition(points.at(i * 3 + 1));
        QPointF p3 = mapPosition(points.at(i * 3 + 2));

        current.setPosition(tmp.position());
        current.setLeftHandle(tmp.leftHandle());
        current.setRightHandle(p1);

        m_frames.push_back(current);

        current.setInterpolation(tmp.interpolation());

        tmp.setLeftHandle(p2);
        tmp.setPosition(p3);
    }

    m_frames.push_back(tmp);

    analyze();
}

bool AnimationCurve::isEmpty() const
{
    return m_frames.empty();
}

bool AnimationCurve::isValid() const
{
    return m_frames.size() >= 2;
}

bool AnimationCurve::isFromData() const
{
    return m_fromData;
}

bool AnimationCurve::hasUnified() const
{
    for (auto &&frame : m_frames) {
        if (frame.isUnified())
            return true;
    }
    return false;
}

AnimationCurve::ValueType AnimationCurve::valueType() const
{
    return m_type;
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

std::string AnimationCurve::string() const
{
    std::stringstream sstream;
    sstream << "{ ";
    for (size_t i = 0; i < m_frames.size(); ++i) {
        if (i == m_frames.size() - 1)
            sstream << m_frames[i].string();
        else
            sstream << m_frames[i].string() << ", ";
    }
    sstream << " }";

    return sstream.str();
}

QString AnimationCurve::unifyString() const
{
    QString out;
    for (auto &&frame : m_frames) {
        if (frame.isUnified())
            out.append("1");
        else
            out.append("0");
    }
    return out;
}

CurveSegment AnimationCurve::segment(double time) const
{
    CurveSegment seg;
    for (auto &frame : m_frames) {
        if (frame.position().x() > time) {
            seg.setRight(frame);
            return seg;
        }
        seg.setLeft(frame);
    }
    return CurveSegment();
}

std::vector<CurveSegment> AnimationCurve::segments() const
{
    if (m_frames.empty())
        return {};

    std::vector<CurveSegment> out;

    CurveSegment current;
    current.setLeft(m_frames.at(0));

    for (size_t i = 1; i < m_frames.size(); ++i) {
        current.setRight(m_frames[i]);
        out.push_back(current);
        current.setLeft(m_frames[i]);
    }
    return out;
}

QPointF mapEasing(const QPointF &start, const QPointF &end, const QPointF &pos)
{
    QPointF slope(end.x() - start.x(), end.y() - start.y());
    return QPointF(start.x() + slope.x() * pos.x(), start.y() + slope.y() * pos.y());
}

QPainterPath AnimationCurve::simplePath() const
{
    if (m_frames.empty())
        return QPainterPath();

    QPainterPath path(m_frames.front().position());

    CurveSegment segment;
    segment.setLeft(m_frames.front());

    for (size_t i = 1; i < m_frames.size(); ++i) {
        segment.setRight(m_frames[i]);
        segment.extend(path);
        segment.setLeft(m_frames[i]);
    }

    return path;
}

QPainterPath AnimationCurve::intersectionPath() const
{
    QPainterPath path = simplePath();
    QPainterPath reversed = path.toReversed();
    path.connectPath(reversed);
    return path;
}

Keyframe AnimationCurve::keyframeAt(size_t id) const
{
    if (id >= m_frames.size())
        return Keyframe();

    return m_frames.at(id);
}

std::vector<Keyframe> AnimationCurve::keyframes() const
{
    return m_frames;
}

std::vector<QPointF> AnimationCurve::extrema() const
{
    std::vector<QPointF> out;

    for (auto &&segment : segments()) {
        const auto es = segment.extrema();
        out.insert(std::end(out), std::begin(es), std::end(es));
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

bool AnimationCurve::intersects(const QPointF &coord, double radiusX, double radiusY) const
{
    if (m_frames.size() < 2)
        return false;

    std::vector<CurveSegment> influencer;

    CurveSegment current;
    current.setLeft(m_frames.at(0));

    for (size_t i = 1; i < m_frames.size(); ++i) {
        const Keyframe &frame = m_frames.at(i);

        current.setRight(frame);

        if (current.containsX(coord.x() - radiusX) || current.containsX(coord.x())
            || current.containsX(coord.x() + radiusX)) {
            influencer.push_back(current);
        }

        if (frame.position().x() > coord.x() + radiusX)
            break;

        current.setLeft(frame);
    }

    for (auto &segment : influencer) {
        if (segment.intersects(coord, radiusX, radiusY))
            return true;
    }
    return false;
}

void AnimationCurve::append(const AnimationCurve &other)
{
    if (!other.isValid())
        return;

    if (!isValid()) {
        m_frames = other.keyframes();
        analyze();
        return;
    }

    std::vector<Keyframe> otherFrames = other.keyframes();
    m_frames.back().setRightHandle(otherFrames.front().rightHandle());
    m_frames.insert(std::end(m_frames), std::begin(otherFrames) + 1, std::end(otherFrames));
    analyze();
}

void AnimationCurve::insert(double time)
{
    CurveSegment seg = segment(time);

    if (!seg.isValid())
        return;

    auto insertFrames = [this](std::array<Keyframe, 3> &&frames) {
        auto samePosition = [frames](const Keyframe &frame) {
            return frame.position() == frames[0].position();
        };

        auto iter = std::find_if(m_frames.begin(), m_frames.end(), samePosition);
        if (iter != m_frames.end()) {
            auto erased = m_frames.erase(iter, iter + 2);
            m_frames.insert(erased, frames.begin(), frames.end());
        }
    };

    insertFrames(seg.splitAt(time));
}

void AnimationCurve::analyze()
{
    m_minY = std::numeric_limits<double>::max();
    m_maxY = std::numeric_limits<double>::lowest();

    auto byTime = [](const auto &a, const auto &b) {
        return a.position().x() < b.position().x();
    };
    std::sort(m_frames.begin(), m_frames.end(), byTime);

    for (auto e : extrema()) {
        if (m_minY > e.y())
            m_minY = e.y();

        if (m_maxY < e.y())
            m_maxY = e.y();
    }

    for (auto &frame : std::as_const(m_frames)) {
        if (frame.position().y() < m_minY)
            m_minY = frame.position().y();

        if (frame.position().y() > m_maxY)
            m_maxY = frame.position().y();

        if (frame.hasLeftHandle()) {
            if (frame.leftHandle().y() < m_minY)
                m_minY = frame.leftHandle().y();

            if (frame.leftHandle().y() > m_maxY)
                m_maxY = frame.leftHandle().y();
        }

        if (frame.hasRightHandle()) {
            if (frame.rightHandle().y() < m_minY)
                m_minY = frame.rightHandle().y();

            if (frame.rightHandle().y() > m_maxY)
                m_maxY = frame.rightHandle().y();
        }
    }
}

} // End namespace QmlDesigner.
