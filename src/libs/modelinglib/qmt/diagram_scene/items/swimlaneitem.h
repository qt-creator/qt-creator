// Copyright (C) 2017 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/selectable.h"

namespace qmt {

class DSwimlane;
class DiagramSceneModel;
class Style;

class SwimlaneItem :
        public QGraphicsItem,
        public IMoveable,
        public ISelectable
{
public:
    SwimlaneItem(DSwimlane *swimlane, DiagramSceneModel *diagramSceneModel,
                 QGraphicsItem *parent = nullptr);
    ~SwimlaneItem() override;

    DSwimlane *swimlane() const { return m_swimlane; }
    DiagramSceneModel *diagramSceneModel() const { return m_diagramSceneModel; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void update();

    void moveDelta(const QPointF &delta) override;
    void alignItemPositionToRaster(double rasterWidth, double rasterHeight) override;

    bool isSecondarySelected() const override;
    void setSecondarySelected(bool secondarySelected) override;
    bool isFocusSelected() const override;
    void setFocusSelected(bool focusSelected) override;
    QRectF getSecondarySelectionBoundary() override;
    void setBoundarySelected(const QRectF &boundary, bool secondary) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void updateSelectionMarker();
    const Style *adaptedStyle();

private:
    QSizeF calcMinimumGeometry() const;
    void updateGeometry();

    DSwimlane *m_swimlane = nullptr;
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    QGraphicsLineItem *m_lineItem = nullptr;
    QGraphicsRectItem *m_selectionMarker = nullptr;
    bool m_isUpdating = false;
    bool m_secondarySelected = false;
    bool m_selectSecondary = false;
};

} // namespace qmt
