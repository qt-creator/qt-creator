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

#include <QEasingCurve>
#include <QMetaType>
#include <QPainterPath>
#include <QPointF>
#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

class EasingCurve : public QEasingCurve
{
public:
    EasingCurve();

    EasingCurve(const QEasingCurve &curve);

    EasingCurve(const EasingCurve &curve);

    EasingCurve(const QPointF &start, const QVector<QPointF> &points);

    virtual ~EasingCurve();

    EasingCurve &operator=(const EasingCurve &curve);

    static bool IsValidIndex(int idx);

    static void registerStreamOperators();

public:
    int count() const;

    int active() const;

    int segmentCount() const;

    bool hasActive() const;

    bool isLegal() const;

    bool isValidIndex(int idx) const;

    bool isSmooth(int idx) const;

    bool isHandle(int idx) const;

    bool isLeftHandle(int idx) const;

    QString toString() const;

    QPointF start() const;

    QPointF end() const;

    QPainterPath path() const;

    int curvePoint(int idx) const;

    QPointF point(int idx) const;

    int hit(const QPointF &point, double threshold) const;

public:
    void makeDefault();

    void clearActive();

    void setActive(int idx);

    void makeSmooth(int idx);

    void breakTangent(int idx);

    void addPoint(const QPointF &point);

    void setPoint(int idx, const QPointF &point);

    void movePoint(int idx, const QPointF &vector);

    void deletePoint(int idx);

    bool fromString(const QString &string);

    void fromCubicSpline(const QVector<QPointF> &points);

    friend QDebug &operator<<(QDebug &stream, const EasingCurve &curve);

    friend QDataStream &operator<<(QDataStream &stream, const EasingCurve &curve);

    friend QDataStream &operator>>(QDataStream &stream, EasingCurve &curve);

    friend std::ostream &operator<<(std::ostream &stream, const EasingCurve &curve);

    friend std::istream &operator>>(std::istream &stream, EasingCurve &curve);

private:
    int m_active;

    QPointF m_start;

    std::vector<int> m_smoothIds;
};

class NamedEasingCurve
{
public:
    NamedEasingCurve();

    NamedEasingCurve(const QString &name, const EasingCurve &curve);

    NamedEasingCurve(const NamedEasingCurve &other);

    NamedEasingCurve &operator=(const NamedEasingCurve &other) = default;

    virtual ~NamedEasingCurve();

    QString name() const;

    EasingCurve curve() const;

    friend QDataStream &operator<<(QDataStream &stream, const NamedEasingCurve &curve);

    friend QDataStream &operator>>(QDataStream &stream, NamedEasingCurve &curve);

private:
    QString m_name;

    EasingCurve m_curve;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::EasingCurve);
Q_DECLARE_METATYPE(QmlDesigner::NamedEasingCurve);
