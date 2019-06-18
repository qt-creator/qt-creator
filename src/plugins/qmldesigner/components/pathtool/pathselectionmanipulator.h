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

#pragma once

#include <QList>
#include <QPair>
#include <QPoint>

#include "controlpoint.h"

namespace QmlDesigner {

class PathItem;

struct SelectionPoint
{
    SelectionPoint();
    SelectionPoint(const ControlPoint &controlPoint);
    ControlPoint controlPoint;
    QPointF startPosition;
};

class PathSelectionManipulator
{
public:
    PathSelectionManipulator(PathItem *pathItem);

    void clear();
    void clearSingleSelection();
    void clearMultiSelection();

    void addMultiSelectionControlPoint(const ControlPoint &controlPoint);
    void addSingleControlPoint(const ControlPoint &controlPoint);
    void addSingleControlPointSmartly(const ControlPoint &editPoint);

    QList<SelectionPoint> singleSelectedPoints();
    QList<SelectionPoint> automaticallyAddedSinglePoints();
    QList<SelectionPoint> allSelectionSinglePoints();
    QList<SelectionPoint> multiSelectedPoints();
    QList<SelectionPoint> allSelectionPoints();

    QList<ControlPoint> allControlPoints();

    bool hasSingleSelection() const;
    bool hasMultiSelection() const;

    void startMultiSelection(const QPointF &startPoint);
    void updateMultiSelection(const QPointF &updatePoint);
    void endMultiSelection();
    QPointF multiSelectionStartPoint() const;
    bool isMultiSelecting() const;

    QRectF multiSelectionRectangle() const;

    void setStartPoint(const QPointF &startPoint);
    QPointF startPoint() const;
    void startMoving(const QPointF &startPoint);
    void updateMoving(const QPointF &updatePoint, Qt::KeyboardModifiers keyboardModifier);
    void endMoving();
    bool isMoving() const;

    void updateMultiSelectedStartPoint();

private:
    QList<SelectionPoint> m_singleSelectedPoints;
    QList<SelectionPoint> m_automaticallyAddedSinglePoints;
    QList<SelectionPoint> m_multiSelectedPoints;
    QPointF m_startPoint;
    QPointF m_updatePoint;
    PathItem *m_pathItem;
    bool m_isMultiSelecting;
    bool m_isMoving;
};

bool operator ==(const SelectionPoint& firstSelectionPoint, const SelectionPoint& secondSelectionPoint);

} // namespace QmlDesigner
