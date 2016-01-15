/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cubicsegment.h"

#include <qmath.h>
#include <QtDebug>


namespace QmlDesigner {

CubicSegment::CubicSegment()
{
}

CubicSegment CubicSegment::create()
{
    CubicSegment cubicSegment;
    cubicSegment.d = new CubicSegmentData;

    return cubicSegment;
}

void CubicSegment::setModelNode(const ModelNode &modelNode)
{
    d->modelNode = modelNode;
}

ModelNode CubicSegment::modelNode() const
{
    return d->modelNode;
}

void CubicSegment::setFirstControlPoint(const ControlPoint &firstControlPoint)
{
    d->firstControllPoint = firstControlPoint;
}

void CubicSegment::setFirstControlPoint(double x, double y)
{
    d->firstControllPoint.setX(x);
    d->firstControllPoint.setY(y);
}

void CubicSegment::setFirstControlPoint(const QPointF &coordiante)
{
    d->firstControllPoint.setCoordinate(coordiante);
}

void CubicSegment::setSecondControlPoint(const ControlPoint &secondControlPoint)
{
    d->secondControllPoint = secondControlPoint;
    d->secondControllPoint.setPathElementModelNode(d->modelNode);
    d->secondControllPoint.setPointType(FirstControlPoint);
}

void CubicSegment::setSecondControlPoint(double x, double y)
{
    d->secondControllPoint.setX(x);
    d->secondControllPoint.setY(y);
    d->secondControllPoint.setPathElementModelNode(d->modelNode);
    d->secondControllPoint.setPointType(FirstControlPoint);
}

void CubicSegment::setSecondControlPoint(const QPointF &coordiante)
{
    d->secondControllPoint.setCoordinate(coordiante);
    d->secondControllPoint.setPathElementModelNode(d->modelNode);
    d->secondControllPoint.setPointType(FirstControlPoint);
}

void CubicSegment::setThirdControlPoint(const ControlPoint &thirdControlPoint)
{
    d->thirdControllPoint = thirdControlPoint;
    d->thirdControllPoint.setPathElementModelNode(d->modelNode);
    d->thirdControllPoint.setPointType(SecondControlPoint);
}

void CubicSegment::setThirdControlPoint(double x, double y)
{
    d->thirdControllPoint.setX(x);
    d->thirdControllPoint.setY(y);
    d->thirdControllPoint.setPathElementModelNode(d->modelNode);
    d->thirdControllPoint.setPointType(SecondControlPoint);
}

void CubicSegment::setThirdControlPoint(const QPointF &coordiante)
{
    d->thirdControllPoint.setCoordinate(coordiante);
    d->thirdControllPoint.setPathElementModelNode(d->modelNode);
    d->thirdControllPoint.setPointType(SecondControlPoint);
}

void CubicSegment::setFourthControlPoint(const ControlPoint &fourthControlPoint)
{
    d->fourthControllPoint = fourthControlPoint;
    d->fourthControllPoint.setPathElementModelNode(d->modelNode);
    d->fourthControllPoint.setPointType(EndPoint);
}

void CubicSegment::setFourthControlPoint(double x, double y)
{
    d->fourthControllPoint.setX(x);
    d->fourthControllPoint.setY(y);
    d->fourthControllPoint.setPathElementModelNode(d->modelNode);
    d->fourthControllPoint.setPointType(EndPoint);
}

void CubicSegment::setFourthControlPoint(const QPointF &coordiante)
{
    d->fourthControllPoint.setCoordinate(coordiante);
    d->fourthControllPoint.setPathElementModelNode(d->modelNode);
    d->fourthControllPoint.setPointType(EndPoint);
}

void CubicSegment::setAttributes(const QMap<QString, QVariant> &attributes)
{
    d->attributes = attributes;
}

void CubicSegment::setPercent(double percent)
{
    d->percent = percent;
}

ControlPoint CubicSegment::firstControlPoint() const
{
    return d->firstControllPoint;
}

ControlPoint CubicSegment::secondControlPoint() const
{
    return d->secondControllPoint;
}

ControlPoint CubicSegment::thirdControlPoint() const
{
    return d->thirdControllPoint;
}

ControlPoint CubicSegment::fourthControlPoint() const
{
    return d->fourthControllPoint;
}

const QMap<QString, QVariant> CubicSegment::attributes() const
{
    return d->attributes;
}

double CubicSegment::percent() const
{
    return d->percent;
}

QList<ControlPoint> CubicSegment::controlPoints() const
{
    QList<ControlPoint> controlPointList;

    controlPointList.reserve(4);

    controlPointList.append(firstControlPoint());
    controlPointList.append(secondControlPoint());
    controlPointList.append(thirdControlPoint());
    controlPointList.append(fourthControlPoint());

    return controlPointList;
}

double CubicSegment::firstControlX() const
{
    return firstControlPoint().coordinate().x();
}

double CubicSegment::firstControlY() const
{
    return firstControlPoint().coordinate().y();
}

double CubicSegment::secondControlX() const
{
    return secondControlPoint().coordinate().x();
}

double CubicSegment::secondControlY() const
{
    return secondControlPoint().coordinate().y();
}

double CubicSegment::thirdControlX() const
{
    return thirdControlPoint().coordinate().x();
}

double CubicSegment::thirdControlY() const
{
    return thirdControlPoint().coordinate().y();
}

double CubicSegment::fourthControlX() const
{
    return fourthControlPoint().coordinate().x();
}

double CubicSegment::fourthControlY() const
{
    return fourthControlPoint().coordinate().y();
}

double CubicSegment::quadraticControlX() const
{
    return -0.25 * firstControlX() + 0.75 * secondControlX() + 0.75 * thirdControlX() - 0.25 * fourthControlX();
}

double CubicSegment::quadraticControlY() const
{
    return -0.25 * firstControlY() + 0.75 * secondControlY() + 0.75 * thirdControlY() - 0.25 * fourthControlY();
}

bool CubicSegment::isValid() const
{
    return d.data();
}

bool CubicSegment::canBeConvertedToLine() const
{
    return canBeConvertedToQuad()
            && qFuzzyIsNull(((3. * d->firstControllPoint.coordinate())
                             - (6. * d->secondControllPoint.coordinate())
                             + (3. * d->thirdControllPoint.coordinate())).manhattanLength());;
}

bool CubicSegment::canBeConvertedToQuad() const
{
    return qFuzzyIsNull(((3. * d->secondControllPoint.coordinate())
                         - (3 * d->thirdControllPoint.coordinate())
                         + d->fourthControllPoint.coordinate()
                         - d->firstControllPoint.coordinate()).manhattanLength());
}

QPointF CubicSegment::sample(double t) const
{
    return qPow(1.-t, 3.) * firstControlPoint().coordinate()
            + 3 * qPow(1.-t, 2.) * t * secondControlPoint().coordinate()
            + 3 * qPow(t, 2.) * (1. - t) * thirdControlPoint().coordinate()
            + qPow(t, 3.) * fourthControlPoint().coordinate();
}

double CubicSegment::minimumDistance(const QPointF &pickPoint, double &tReturnValue) const
{
    double actualMinimumDistance = 10000000.;
    for (double t = 0.0; t <= 1.0; t += 0.1) {
        QPointF samplePoint = sample(t);
        QPointF distanceVector = pickPoint - samplePoint;
        if (distanceVector.manhattanLength() < actualMinimumDistance) {
            actualMinimumDistance = distanceVector.manhattanLength();
            tReturnValue = t;
        }
    }

    return actualMinimumDistance;
}

static QPointF interpolatedPoint(double t, const QPointF &firstPoint, const QPointF &secondPoint)
{
    return (secondPoint - firstPoint) * t + firstPoint;
}

QPair<CubicSegment, CubicSegment> CubicSegment::split(double t)
{
    // first pass
    QPointF secondPointFirstSegment = interpolatedPoint(t, firstControlPoint().coordinate(), secondControlPoint().coordinate());
    QPointF firstIntermediatPoint = interpolatedPoint(t, secondControlPoint().coordinate(), thirdControlPoint().coordinate());
    QPointF thirdPointSecondSegment = interpolatedPoint(t, thirdControlPoint().coordinate(), fourthControlPoint().coordinate());

    // second pass
    QPointF thirdPointFirstSegment = interpolatedPoint(t, secondPointFirstSegment, firstIntermediatPoint);
    QPointF secondPointSecondSegment = interpolatedPoint(t, firstIntermediatPoint, thirdPointSecondSegment);

    // third pass
    QPointF midPoint = interpolatedPoint(t, thirdPointFirstSegment, secondPointSecondSegment);
    ControlPoint midControlPoint(midPoint);


    CubicSegment firstCubicSegment = CubicSegment::create();
    firstCubicSegment.setFirstControlPoint(firstControlPoint().coordinate());
    firstCubicSegment.setSecondControlPoint(secondPointFirstSegment);
    firstCubicSegment.setThirdControlPoint(thirdPointFirstSegment);
    firstCubicSegment.setFourthControlPoint(midControlPoint);

    CubicSegment secondCubicSegment =  CubicSegment::create();
    secondCubicSegment.setFirstControlPoint(midControlPoint);
    secondCubicSegment.setSecondControlPoint(secondPointSecondSegment);
    secondCubicSegment.setThirdControlPoint(thirdPointSecondSegment);
    secondCubicSegment.setFourthControlPoint(fourthControlPoint().coordinate());

    qDebug() << firstCubicSegment << secondCubicSegment;

    return qMakePair(firstCubicSegment, secondCubicSegment);
}

void CubicSegment::makeStraightLine()
{
    QPointF lineVector = fourthControlPoint().coordinate() - firstControlPoint().coordinate();
    QPointF newSecondControlPoint = firstControlPoint().coordinate() + (lineVector * 0.3);
    QPointF newThirdControlPoint = fourthControlPoint().coordinate() - (lineVector * 0.3);
    setSecondControlPoint(newSecondControlPoint);
    setThirdControlPoint(newThirdControlPoint);
}

void CubicSegment::updateModelNode()
{
    firstControlPoint().updateModelNode();
    secondControlPoint().updateModelNode();
    thirdControlPoint().updateModelNode();
    fourthControlPoint().updateModelNode();
}

CubicSegmentData::CubicSegmentData()
    : firstControllPoint(0., 0.),
      secondControllPoint(0., 0.),
      thirdControllPoint(0., 0.),
      fourthControllPoint(0., 0.),
      percent(-1.0)
{
}

bool operator ==(const CubicSegment& firstCubicSegment, const CubicSegment& secondCubicSegment)
{
    return firstCubicSegment.d.data() == secondCubicSegment.d.data();
}

QDebug operator<<(QDebug debug, const CubicSegment &cubicSegment)
{
    if (cubicSegment.isValid()) {
        debug.nospace() << "CubicSegment("
                << cubicSegment.firstControlPoint() << ", "
                << cubicSegment.secondControlPoint() << ", "
                << cubicSegment.thirdControlPoint() << ", "
                << cubicSegment.fourthControlPoint() << ')';
    } else {
        debug.nospace() << "CubicSegment(invalid)";
    }

    return debug.space();
}
} // namespace QmlDesigner
