// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "controlpoint.h"

#include <QtDebug>

#include <variantproperty.h>

#include <rewritertransaction.h>

namespace QmlDesigner {

ControlPoint::ControlPoint() = default;

ControlPoint::ControlPoint(const ControlPoint &other) = default;

ControlPoint::ControlPoint(const QPointF &coordinate)
    : d(new ControlPointData)
{
     d->coordinate = coordinate;
}

ControlPoint::ControlPoint(double x, double y)
    : d(new ControlPointData)
{
    d->coordinate = QPointF(x, y);
}

ControlPoint::~ControlPoint() = default;

ControlPoint &ControlPoint::operator =(const ControlPoint &other)
{
    if (d != other.d)
        d = other.d;

    return *this;
}

void ControlPoint::setX(double x)
{
    d->coordinate.setX(x);
}

void ControlPoint::setY(double y)
{
    d->coordinate.setY(y);
}

void ControlPoint::setCoordinate(const QPointF &coordinate)
{
    d->coordinate = coordinate;
}

void ControlPoint::setPathElementModelNode(const ModelNode &modelNode)
{
    d->pathElementModelNode = modelNode;
}

ModelNode ControlPoint::pathElementModelNode() const
{
    return d->pathElementModelNode;
}

void ControlPoint::setPathModelNode(const ModelNode &pathModelNode)
{
    d->pathModelNode = pathModelNode;
}

ModelNode ControlPoint::pathModelNode() const
{
    return d->pathModelNode;
}

void ControlPoint::setPointType(PointType pointType)
{
    d->pointType = pointType;
}

PointType ControlPoint::pointType() const
{
    return d->pointType;
}

QPointF ControlPoint::coordinate() const
{
    return d->coordinate;
}

bool ControlPoint::isValid() const
{
    return d.data();
}

bool ControlPoint::isEditPoint() const
{
    return isValid() && (pointType() == StartPoint || pointType() == EndPoint);
}

bool ControlPoint::isControlVertex() const
{
    return isValid() && (pointType() == FirstControlPoint || pointType() == SecondControlPoint);
}

void ControlPoint::updateModelNode()
{
    switch (pointType()) {
    case StartPoint:
        d->pathModelNode.variantProperty("startX").setValue(coordinate().x());
        d->pathModelNode.variantProperty("startY").setValue(coordinate().y());
        break;
    case FirstControlPoint:
        d->pathElementModelNode.variantProperty("control1X").setValue(coordinate().x());
        d->pathElementModelNode.variantProperty("control1Y").setValue(coordinate().y());
        break;
    case SecondControlPoint:
        d->pathElementModelNode.variantProperty("control2X").setValue(coordinate().x());
        d->pathElementModelNode.variantProperty("control2Y").setValue(coordinate().y());
        break;
    case EndPoint:
        d->pathElementModelNode.variantProperty("x").setValue(coordinate().x());
        d->pathElementModelNode.variantProperty("y").setValue(coordinate().y());
        break;
    case StartAndEndPoint:
        d->pathElementModelNode.variantProperty("x").setValue(coordinate().x());
        d->pathElementModelNode.variantProperty("y").setValue(coordinate().y());
        d->pathModelNode.variantProperty("startX").setValue(coordinate().x());
        d->pathModelNode.variantProperty("startY").setValue(coordinate().y());
        break;
    }
}

bool operator ==(const ControlPoint& firstControlPoint, const ControlPoint& secondControlPoint)
{
    return firstControlPoint.d.data() == secondControlPoint.d.data() && firstControlPoint.d.data();
}

QDebug operator<<(QDebug debug, const ControlPoint &controlPoint)
{
    if (controlPoint.isValid()) {
        debug.nospace() << "ControlPoint("
                << controlPoint.coordinate().x() << ", "
                << controlPoint.coordinate().y() << ", "
                << controlPoint.pointType() << ')';
    } else {
        debug.nospace() << "ControlPoint(invalid)";
    }

    return debug.space();
}

} // namespace QmlDesigner
