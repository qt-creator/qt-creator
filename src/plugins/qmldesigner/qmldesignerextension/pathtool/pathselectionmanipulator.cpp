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

#include "pathselectionmanipulator.h"

#include "pathitem.h"

#include <QtDebug>

namespace QmlDesigner {

PathSelectionManipulator::PathSelectionManipulator(PathItem *pathItem)
    : m_pathItem(pathItem),
      m_isMultiSelecting(false),
      m_isMoving(false)
{
}

void PathSelectionManipulator::clear()
{
    clearSingleSelection();
    clearMultiSelection();
    m_isMultiSelecting = false;
    m_isMoving = false;
}

void PathSelectionManipulator::clearSingleSelection()
{
    m_singleSelectedPoints.clear();
    m_automaticallyAddedSinglePoints.clear();
}

void PathSelectionManipulator::clearMultiSelection()
{
    m_multiSelectedPoints.clear();
}

static SelectionPoint createSelectionPoint(const ControlPoint &controlPoint)
{
    SelectionPoint selectionPoint;
    selectionPoint.controlPoint = controlPoint;
    selectionPoint.startPosition = controlPoint.coordinate();

    return selectionPoint;
}

void PathSelectionManipulator::addMultiSelectionControlPoint(const ControlPoint &controlPoint)
{
    m_multiSelectedPoints.append(createSelectionPoint(controlPoint));
}

static ControlPoint getControlPoint(const QList<ControlPoint> &selectedPoints, const ControlPoint &controlPoint, int indexOffset, bool isClosedPath)
{
    int controlPointIndex = selectedPoints.indexOf(controlPoint);
    if (controlPointIndex >= 0) {
        int offsetIndex = controlPointIndex + indexOffset;
        if (offsetIndex >= 0 && offsetIndex < selectedPoints.count())
            return selectedPoints.at(offsetIndex);
        else if (isClosedPath) {
            if (offsetIndex == -1)
                return selectedPoints.last();
            else if (offsetIndex < selectedPoints.count())
                return selectedPoints.at(1);
        }
    }

    return ControlPoint();
}

QList<SelectionPoint> PathSelectionManipulator::singleSelectedPoints()
{
    return m_singleSelectedPoints;
}

QList<SelectionPoint> PathSelectionManipulator::automaticallyAddedSinglePoints()
{
    return m_automaticallyAddedSinglePoints;
}

QList<SelectionPoint> PathSelectionManipulator::allSelectionSinglePoints()
{

    return m_singleSelectedPoints + m_automaticallyAddedSinglePoints;
}

QList<SelectionPoint> PathSelectionManipulator::multiSelectedPoints()
{
    return m_multiSelectedPoints;
}

QList<SelectionPoint> PathSelectionManipulator::allSelectionPoints()
{
    return m_singleSelectedPoints + m_multiSelectedPoints + m_automaticallyAddedSinglePoints;
}

QList<ControlPoint> PathSelectionManipulator::allControlPoints()
{
    QList<ControlPoint> controlPoints;

    foreach (const SelectionPoint &selectionPoint, m_singleSelectedPoints)
        controlPoints.append(selectionPoint.controlPoint);

    foreach (const SelectionPoint &selectionPoint, m_automaticallyAddedSinglePoints)
        controlPoints.append(selectionPoint.controlPoint);

    foreach (const SelectionPoint &selectionPoint, m_multiSelectedPoints)
        controlPoints.append(selectionPoint.controlPoint);

    return controlPoints;
}

bool PathSelectionManipulator::hasSingleSelection() const
{
    return !m_singleSelectedPoints.isEmpty();
}

bool PathSelectionManipulator::hasMultiSelection() const
{
    return !m_multiSelectedPoints.isEmpty();
}

void PathSelectionManipulator::startMultiSelection(const QPointF &startPoint)
{
    m_startPoint = startPoint;
    m_isMultiSelecting = true;
}

void PathSelectionManipulator::updateMultiSelection(const QPointF &updatePoint)
{
    clearMultiSelection();

    m_updatePoint = updatePoint;

    QRectF selectionRect(m_startPoint, updatePoint);

    foreach (const ControlPoint &controlPoint, m_pathItem->controlPoints()) {
        if (selectionRect.contains(controlPoint.coordinate()))
            addMultiSelectionControlPoint(controlPoint);
    }
}

void PathSelectionManipulator::endMultiSelection()
{
    m_isMultiSelecting = false;
}

SelectionPoint::SelectionPoint()
{
}

SelectionPoint::SelectionPoint(const ControlPoint &controlPoint)
    : controlPoint(controlPoint)
{
}

bool operator ==(const SelectionPoint &firstSelectionPoint, const SelectionPoint &secondSelectionPoint)
{
    return firstSelectionPoint.controlPoint == secondSelectionPoint.controlPoint;
}

QPointF PathSelectionManipulator::multiSelectionStartPoint() const
{
    return m_startPoint;
}

bool PathSelectionManipulator::isMultiSelecting() const
{
    return m_isMultiSelecting;
}

QRectF PathSelectionManipulator::multiSelectionRectangle() const
{
    return QRectF(m_startPoint, m_updatePoint);
}

void PathSelectionManipulator::setStartPoint(const QPointF &startPoint)
{
    m_startPoint = startPoint;
}

QPointF PathSelectionManipulator::startPoint() const
{
    return m_startPoint;
}

double snapFactor(Qt::KeyboardModifiers keyboardModifier)
{
    if (keyboardModifier.testFlag(Qt::ControlModifier))
        return 10.;

    return 1.;
}

QPointF roundedVector(const QPointF &vector, double factor = 1.)
{
    QPointF roundedPosition;

    roundedPosition.setX(qRound(vector.x() / factor) * factor);
    roundedPosition.setY(qRound(vector.y() / factor) * factor);

    return roundedPosition;
}

QPointF manipulatedVector(const QPointF &vector, Qt::KeyboardModifiers keyboardModifier)
{
    QPointF manipulatedVector = roundedVector(vector, snapFactor(keyboardModifier));

    if (keyboardModifier.testFlag(Qt::ShiftModifier))
        manipulatedVector.setX(0.);

    if (keyboardModifier.testFlag(Qt::AltModifier))
        manipulatedVector.setY(0.);

    return manipulatedVector;
}

static void moveControlPoints(const QList<SelectionPoint> &movePoints, const QPointF &offsetVector)
{
    foreach (SelectionPoint movePoint, movePoints)
        movePoint.controlPoint.setCoordinate(movePoint.startPosition + offsetVector);
}

void PathSelectionManipulator::startMoving(const QPointF &startPoint)
{
    m_isMoving = true;
    m_startPoint = startPoint;
}

void PathSelectionManipulator::updateMoving(const QPointF &updatePoint, Qt::KeyboardModifiers keyboardModifier)
{
    m_updatePoint = updatePoint;
    QPointF offsetVector = manipulatedVector(updatePoint - m_startPoint, keyboardModifier) ;
    moveControlPoints(allSelectionPoints(), offsetVector);
}

void PathSelectionManipulator::endMoving()
{
    updateMultiSelectedStartPoint();
    m_isMoving = false;
}

bool PathSelectionManipulator::isMoving() const
{
    return m_isMoving;
}

void PathSelectionManipulator::updateMultiSelectedStartPoint()
{
    QList<SelectionPoint> oldSelectionPoints = m_multiSelectedPoints;

    m_multiSelectedPoints.clear();

    foreach (SelectionPoint selectionPoint, oldSelectionPoints) {
        selectionPoint.startPosition = selectionPoint.controlPoint.coordinate();
        m_multiSelectedPoints.append(selectionPoint);
    }
}

void PathSelectionManipulator::addSingleControlPoint(const ControlPoint &controlPoint)
{
    m_singleSelectedPoints.append(createSelectionPoint(controlPoint));
}

void PathSelectionManipulator::addSingleControlPointSmartly(const ControlPoint &editPoint)
{
    m_singleSelectedPoints.append(createSelectionPoint(editPoint));

    if (editPoint.isEditPoint()) {
        ControlPoint previousControlPoint = getControlPoint(m_pathItem->controlPoints(), editPoint, -1, m_pathItem->isClosedPath());
        if (previousControlPoint.isValid())
            m_automaticallyAddedSinglePoints.append(createSelectionPoint(previousControlPoint));

        ControlPoint nextControlPoint= getControlPoint(m_pathItem->controlPoints(), editPoint, 1, m_pathItem->isClosedPath());
        if (nextControlPoint.isValid())
            m_automaticallyAddedSinglePoints.append(createSelectionPoint(nextControlPoint));
    }
}

} // namespace QMlDesigner
