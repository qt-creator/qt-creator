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

#include "easingcurve.h"
#include "timelineutils.h"

#include <QLineF>

#include <utils/qtcassert.h>

namespace QmlDesigner {

EasingCurve::EasingCurve()
    : QEasingCurve(QEasingCurve::BezierSpline)
    , m_active(-1)
    , m_start(0.0, 0.0)
{}

EasingCurve::EasingCurve(const QEasingCurve &curve)
    : QEasingCurve(curve)
    , m_active(-1)
    , m_start(0.0, 0.0)
{}

EasingCurve::EasingCurve(const EasingCurve &curve) = default;

EasingCurve::EasingCurve(const QPointF &start, const QVector<QPointF> &points)
    : QEasingCurve(QEasingCurve::BezierSpline)
    , m_active(-1)
    , m_start(start)
{
    fromCubicSpline(points);
}

EasingCurve &EasingCurve::operator=(const EasingCurve &curve) = default;

EasingCurve::~EasingCurve() = default;

bool EasingCurve::IsValidIndex(int idx)
{
    return idx >= 0;
}

void EasingCurve::registerStreamOperators()
{
    qRegisterMetaType<QmlDesigner::EasingCurve>("QmlDesigner::EasingCurve");
    qRegisterMetaType<QmlDesigner::NamedEasingCurve>("QmlDesigner::NamedEasingCurve");
    qRegisterMetaTypeStreamOperators<QmlDesigner::EasingCurve>("QmlDesigner::EasingCurve");
    qRegisterMetaTypeStreamOperators<QmlDesigner::NamedEasingCurve>("QmlDesigner::NamedEasingCurve");
}

int EasingCurve::count() const
{
    return toCubicSpline().count();
}

int EasingCurve::active() const
{
    return m_active;
}

int EasingCurve::segmentCount() const
{
    return toCubicSpline().count() / 3;
}

bool EasingCurve::isLegal() const
{
    QPainterPath painterPath(path());

    double increment = 1.0 / 30.0;
    QPointF max = painterPath.pointAtPercent(0.0);
    for (double i = increment; i <= 1.0; i += increment) {
        QPointF current = painterPath.pointAtPercent(i);
        if (current.x() < max.x())
            return false;
        else
            max = current;
    }
    return true;
}

bool EasingCurve::hasActive() const
{
    QTC_ASSERT(m_active < toCubicSpline().size(), return false);
    return m_active >= 0;
}

bool EasingCurve::isValidIndex(int idx) const
{
    return idx >= 0 && idx < toCubicSpline().size();
}

bool EasingCurve::isSmooth(int idx) const
{
    auto iter = std::find(m_smoothIds.begin(), m_smoothIds.end(), idx);
    return iter != m_smoothIds.end();
}

bool EasingCurve::isHandle(int idx) const
{
    return (idx + 1) % 3;
}

bool EasingCurve::isLeftHandle(int idx) const
{
    return ((idx + 2) % 3) == 0;
}

QString EasingCurve::toString() const
{
    QLatin1Char c(',');
    QString s = QLatin1String("[");
    for (const QPointF &point : toCubicSpline()) {
        auto x = QString::number(point.x(), 'g', 3);
        auto y = QString::number(point.y(), 'g', 3);
        s += x + c + y + c;
    }

    // Replace last "," with "]"
    s.chop(1);
    s.append(QLatin1Char(']'));

    return s;
}

bool EasingCurve::fromString(const QString &code)
{
    if (code.startsWith(QLatin1Char('[')) && code.endsWith(QLatin1Char(']'))) {
        const QStringRef cleanCode(&code, 1, code.size() - 2);
        const auto stringList = cleanCode.split(QLatin1Char(','), QString::SkipEmptyParts);

        if (stringList.count() >= 6 && (stringList.count() % 6 == 0)) {
            bool checkX, checkY;
            QVector<QPointF> points;
            for (int i = 0; i < stringList.count(); ++i) {
                QPointF point;
                point.rx() = stringList[i].toDouble(&checkX);
                point.ry() = stringList[++i].toDouble(&checkY);

                if (!checkX || !checkY)
                    return false;

                points.push_back(point);
            }

            if (points.constLast() != QPointF(1.0, 1.0))
                return false;

            QEasingCurve easingCurve(QEasingCurve::BezierSpline);

            for (int i = 0; i < points.count() / 3; ++i) {
                easingCurve.addCubicBezierSegment(points.at(i * 3),
                                                  points.at(i * 3 + 1),
                                                  points.at(i * 3 + 2));
            }

            fromCubicSpline(easingCurve.toCubicSpline());
            return true;
        }
    }
    return false;
}

QPointF EasingCurve::start() const
{
    return m_start;
}

QPointF EasingCurve::end() const
{
    return toCubicSpline().last();
}

QPainterPath EasingCurve::path() const
{
    QPainterPath path;
    path.moveTo(m_start);

    QVector<QPointF> controlPoints = toCubicSpline();

    int numSegments = controlPoints.count() / 3;
    for (int i = 0; i < numSegments; i++) {
        QPointF p1 = controlPoints.at(i * 3);
        QPointF p2 = controlPoints.at(i * 3 + 1);
        QPointF p3 = controlPoints.at(i * 3 + 2);
        path.cubicTo(p1, p2, p3);
    }

    return path;
}

int EasingCurve::curvePoint(int idx) const
{
    if (isHandle(idx)) {
        if (isLeftHandle(idx))
            return idx + 1;
        else
            return idx - 1;
    }
    return idx;
}

QPointF EasingCurve::point(int idx) const
{
    QVector<QPointF> controlPoints = toCubicSpline();

    QTC_ASSERT(controlPoints.count() > idx || idx < 0, return QPointF());

    return controlPoints.at(idx);
}

int EasingCurve::hit(const QPointF &point, double threshold) const
{
    int id = -1;
    qreal distance = std::numeric_limits<qreal>::max();

    QVector<QPointF> controlPoints = toCubicSpline();
    for (int i = 0; i < controlPoints.size() - 1; ++i) {
        qreal d = QLineF(point, controlPoints.at(i)).length();
        if (d < threshold && d < distance) {
            distance = d;
            id = i;
        }
    }
    return id;
}

void EasingCurve::makeDefault()
{
    QVector<QPointF> controlPoints;
    controlPoints.append(QPointF(0.0, 0.2));
    controlPoints.append(QPointF(0.3, 0.5));
    controlPoints.append(QPointF(0.5, 0.5));

    controlPoints.append(QPointF(0.7, 0.5));
    controlPoints.append(QPointF(1.0, 0.8));
    controlPoints.append(QPointF(1.0, 1.0));

    fromCubicSpline(controlPoints);

    m_smoothIds.push_back(2);
}

void EasingCurve::clearActive()
{
    m_active = -1;
}

void EasingCurve::setActive(int idx)
{
    m_active = idx;
}

void EasingCurve::makeSmooth(int idx)
{
    if (!isSmooth(idx) && !isHandle(idx)) {
        QVector<QPointF> controlPoints = toCubicSpline();

        QPointF before = m_start;
        if (idx > 3)
            before = controlPoints.at(idx - 3);

        QPointF after = end();
        if ((idx + 3) < controlPoints.count())
            after = controlPoints.at(idx + 3);

        QPointF tangent = (after - before) / 6;

        QPointF thisPoint = controlPoints.at(idx);

        if (idx > 0)
            controlPoints[idx - 1] = thisPoint - tangent;

        if (idx + 1 < controlPoints.count())
            controlPoints[idx + 1] = thisPoint + tangent;

        fromCubicSpline(controlPoints);

        m_smoothIds.push_back(idx);
    }
}

void EasingCurve::breakTangent(int idx)
{
    if (isSmooth(idx) && !isHandle(idx)) {
        QVector<QPointF> controlPoints = toCubicSpline();

        QPointF before = m_start;
        if (idx > 3)
            before = controlPoints.at(idx - 3);

        QPointF after = end();
        if ((idx + 3) < controlPoints.count())
            after = controlPoints.at(idx + 3);

        QPointF thisPoint = controlPoints.at(idx);

        if (idx > 0)
            controlPoints[idx - 1] = (before - thisPoint) / 3 + thisPoint;

        if (idx + 1 < controlPoints.count())
            controlPoints[idx + 1] = (after - thisPoint) / 3 + thisPoint;

        fromCubicSpline(controlPoints);

        auto iter = std::find(m_smoothIds.begin(), m_smoothIds.end(), idx);
        m_smoothIds.erase(iter);
    }
}

void EasingCurve::addPoint(const QPointF &point)
{
    QVector<QPointF> controlPoints = toCubicSpline();

    int splitIndex = 0;
    for (int i = 0; i < controlPoints.size() - 1; ++i) {
        if (!isHandle(i)) {
            if (controlPoints.at(i).x() > point.x())
                break;

            splitIndex = i;
        }
    }

    QPointF before = m_start;
    if (splitIndex > 0) {
        before = controlPoints.at(splitIndex);
    }

    QPointF after = end();
    if ((splitIndex + 3) < controlPoints.count()) {
        after = controlPoints.at(splitIndex + 3);
    }

    int newIdx;

    if (splitIndex > 0) {
        newIdx = splitIndex + 3;
        controlPoints.insert(splitIndex + 2, (point + after) / 2);
        controlPoints.insert(splitIndex + 2, point);
        controlPoints.insert(splitIndex + 2, (point + before) / 2);
    } else {
        newIdx = splitIndex + 2;
        controlPoints.insert(splitIndex + 1, (point + after) / 2);
        controlPoints.insert(splitIndex + 1, point);
        controlPoints.insert(splitIndex + 1, (point + before) / 2);
    }

    fromCubicSpline(controlPoints);

    QTC_ASSERT(!isHandle(newIdx), return );

    m_active = newIdx;

    breakTangent(newIdx);
    makeSmooth(newIdx);
}

void EasingCurve::setPoint(int idx, const QPointF &point)
{
    if (!isValidIndex(idx))
        return;

    QVector<QPointF> controlPoints = toCubicSpline();

    controlPoints[idx] = point;

    fromCubicSpline(controlPoints);
}

void EasingCurve::movePoint(int idx, const QPointF &vector)
{
    if (!isValidIndex(idx))
        return;

    QVector<QPointF> controlPoints = toCubicSpline();

    controlPoints[idx] += vector;

    fromCubicSpline(controlPoints);
}

void EasingCurve::deletePoint(int idx)
{
    if (!isValidIndex(idx))
        return;

    QVector<QPointF> controlPoints = toCubicSpline();

    controlPoints.remove(idx - 1, 3);

    fromCubicSpline(controlPoints);
}

void EasingCurve::fromCubicSpline(const QVector<QPointF> &points)
{
    QEasingCurve tmp(QEasingCurve::BezierSpline);

    int numSegments = points.count() / 3;
    for (int i = 0; i < numSegments; ++i) {
        tmp.addCubicBezierSegment(points.at(i * 3), points.at(i * 3 + 1), points.at(i * 3 + 2));
    }
    swap(tmp);
}

QDebug &operator<<(QDebug &stream, const EasingCurve &curve)
{
    stream << static_cast<QEasingCurve>(curve);
    stream << "\"active:" << curve.m_active << "\"";
    stream << "\"smooth ids:" << curve.m_smoothIds << "\"";
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const EasingCurve &curve)
{
    // Ignore the active flag.
    stream << static_cast<QEasingCurve>(curve);
    stream << curve.toCubicSpline();
    stream << curve.m_smoothIds;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, EasingCurve &curve)
{
    // This is to circumvent a bug in QEasingCurve serialization.
    QVector<QPointF> points;

    // Ignore the active flag.
    stream >> static_cast<QEasingCurve &>(curve);
    stream >> points;
    curve.fromCubicSpline(points);
    stream >> curve.m_smoothIds;

    return stream;
}

NamedEasingCurve::NamedEasingCurve()
    : m_name()
    , m_curve()
{}

NamedEasingCurve::NamedEasingCurve(const QString &name, const EasingCurve &curve)
    : m_name(name)
    , m_curve(curve)
{}

NamedEasingCurve::NamedEasingCurve(const NamedEasingCurve &other) = default;

NamedEasingCurve::~NamedEasingCurve() = default;

QString NamedEasingCurve::name() const
{
    return m_name;
}

EasingCurve NamedEasingCurve::curve() const
{
    return m_curve;
}

QDataStream &operator<<(QDataStream &stream, const NamedEasingCurve &curve)
{
    stream << curve.m_name;
    stream << curve.m_curve;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, NamedEasingCurve &curve)
{
    stream >> curve.m_name;
    stream >> curve.m_curve;
    return stream;
}

} // namespace QmlDesigner
