// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "controlpoint.h"

#include <modelnode.h>

#include <QMap>

#include <QPointF>
#include <QExplicitlySharedDataPointer>

namespace QmlDesigner {

class CubicSegmentData : public QSharedData
{
public:
    CubicSegmentData();
    ModelNode modelNode;
    ControlPoint firstControllPoint;
    ControlPoint secondControllPoint;
    ControlPoint thirdControllPoint;
    ControlPoint fourthControllPoint;
    QMap<QString, QVariant> attributes;
    double percent;
};

class CubicSegment
{
    friend bool operator ==(const CubicSegment& firstCubicSegment, const CubicSegment& secondCubicSegment);

public:
    CubicSegment();

    static CubicSegment create();

    void setModelNode(const ModelNode &modelNode);
    ModelNode modelNode() const;

    void setFirstControlPoint(const ControlPoint &firstControlPoint);
    void setFirstControlPoint(double x, double y);
    void setFirstControlPoint(const QPointF &coordiante);

    void setSecondControlPoint(const ControlPoint &secondControlPoint);
    void setSecondControlPoint(double x, double y);
    void setSecondControlPoint(const QPointF &coordiante);

    void setThirdControlPoint(const ControlPoint &thirdControlPoint);
    void setThirdControlPoint(double x, double y);
    void setThirdControlPoint(const QPointF &coordiante);

    void setFourthControlPoint(const ControlPoint &fourthControlPoint);
    void setFourthControlPoint(double x, double y);
    void setFourthControlPoint(const QPointF &coordiante);

    void setAttributes(const QMap<QString, QVariant> &attributes);

    void setPercent(double percent);

    ControlPoint firstControlPoint() const;
    ControlPoint secondControlPoint() const;
    ControlPoint thirdControlPoint() const;
    ControlPoint fourthControlPoint() const;

    const QMap<QString, QVariant> attributes() const;

    double percent() const;

    QList<ControlPoint> controlPoints() const;

    double firstControlX() const;
    double firstControlY() const;
    double secondControlX() const;
    double secondControlY() const;
    double thirdControlX() const;
    double thirdControlY() const;
    double fourthControlX() const;
    double fourthControlY() const;
    double quadraticControlX() const;
    double quadraticControlY() const;

    bool isValid() const;
    bool canBeConvertedToLine() const;
    bool canBeConvertedToQuad() const;

    QPointF sample(double t) const;
    double minimumDistance(const QPointF &pickPoint, double &t) const;

    QPair<CubicSegment, CubicSegment> split(double t);

    void makeStraightLine();

    void updateModelNode();

private:
    QExplicitlySharedDataPointer<CubicSegmentData> d;
};

bool operator ==(const CubicSegment& firstCubicSegment, const CubicSegment& secondCubicSegment);
QDebug operator<<(QDebug debug, const CubicSegment &cubicSegment);

} // namespace QmlDesigner
