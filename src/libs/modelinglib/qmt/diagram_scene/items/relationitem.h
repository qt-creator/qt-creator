// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/selectable.h"
#include "qmt/diagram_scene/capabilities/windable.h"

QT_BEGIN_NAMESPACE
class QGraphicsSimpleTextItem;
class QPainterPath;
QT_END_NAMESPACE

namespace qmt {

class Uid;
class DRelation;
class DiagramSceneModel;
class ArrowItem;
class StereotypesItem;
class PathSelectionItem;
class Style;

class RelationItem :
        public QGraphicsItem,
        public IMoveable,
        public ISelectable,
        public IWindable
{
    class ArrowConfigurator;

public:
    RelationItem(DRelation *relation, DiagramSceneModel *diagramSceneModel,
                 QGraphicsItem *parent = nullptr);
    ~RelationItem() override;

    DRelation *relation() const { return m_relation; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;

    void moveDelta(const QPointF &delta) override;
    void alignItemPositionToRaster(double rasterWidth, double rasterHeight) override;

    bool isSecondarySelected() const override;
    void setSecondarySelected(bool secondarySelected) override;
    bool isFocusSelected() const override;
    void setFocusSelected(bool focusSelected) override;
    QRectF getSecondarySelectionBoundary() override;
    void setBoundarySelected(const QRectF &boundary, bool secondary) override;

    QPointF grabHandle(int index) override;
    void insertHandle(int beforeIndex, const QPointF &pos, double rasterWidth, double rasterHeight) override;
    void deleteHandle(int index) override;
    void setHandlePos(int index, const QPointF &pos) override;
    void dropHandle(int index, double rasterWidth, double rasterHeight) override;

    virtual void update();

protected:
    virtual void update(const Style *style);

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    const Style *adaptedStyle();
    QPointF calcEndPoint(const Uid &end, const Uid &otherEnd, int nearestIntermediatePointIndex);
    QPointF calcEndPoint(const Uid &end, const QPointF &otherEndPos,
                         int nearestIntermediatePointIndex);

    DRelation *m_relation = nullptr;

protected:
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    bool m_isSecondarySelected = false;
    bool m_isFocusSelected = false;
    ArrowItem *m_arrow = nullptr;
    QGraphicsSimpleTextItem *m_name = nullptr;
    StereotypesItem *m_stereotypes = nullptr;
    PathSelectionItem *m_selectionHandles = nullptr;
    static bool m_grabbedEndA;
    static bool m_grabbedEndB;
    static QPointF m_grabbedEndPos;
};

} // namespace qmt
