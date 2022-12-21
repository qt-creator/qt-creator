// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

#include <QPointF>
#include <QExplicitlySharedDataPointer>

namespace QmlDesigner {

enum PointType {
    StartPoint,
    FirstControlPoint,
    SecondControlPoint,
    EndPoint,
    StartAndEndPoint
};

class ControlPointData : public QSharedData
{
public:
    ModelNode pathElementModelNode;
    ModelNode pathModelNode;
    QPointF coordinate;
    PointType pointType;
};

class ControlPoint
{
    friend bool operator ==(const ControlPoint& firstControlPoint, const ControlPoint& secondControlPoint);

public:
    ControlPoint();
    ControlPoint(const ControlPoint &other);
    ControlPoint(const QPointF &coordinate);
    ControlPoint(double x, double y);

    ~ControlPoint();

    ControlPoint &operator =(const ControlPoint &other);

    void setX(double x);
    void setY(double y);
    void setCoordinate(const QPointF &coordinate);
    QPointF coordinate() const;

    void setPathElementModelNode(const ModelNode &pathElementModelNode);
    ModelNode pathElementModelNode() const;

    void setPathModelNode(const ModelNode &pathModelNode);
    ModelNode pathModelNode() const;

    void setPointType(PointType pointType);
    PointType pointType() const;

    bool isValid() const;
    bool isEditPoint() const;
    bool isControlVertex() const;

    void updateModelNode();

private:
    QExplicitlySharedDataPointer<ControlPointData> d;
};

bool operator ==(const ControlPoint& firstControlPoint, const ControlPoint& secondControlPoint);
QDebug operator<<(QDebug debug, const ControlPoint &controlPoint);

} // namespace QmlDesigner
