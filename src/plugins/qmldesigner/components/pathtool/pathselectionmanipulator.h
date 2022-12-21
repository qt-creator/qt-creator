// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
