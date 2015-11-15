/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_GRAPHICSRELATIONITEM_H
#define QMT_GRAPHICSRELATIONITEM_H

#include <QGraphicsItem>
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/selectable.h"
#include "qmt/diagram_scene/capabilities/windable.h"

QT_BEGIN_NAMESPACE
class QGraphicsSimpleTextItem;
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
                 QGraphicsItem *parent = 0);
    ~RelationItem();

    DRelation *relation() const { return m_relation; }

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPainterPath shape() const;

    void moveDelta(const QPointF &delta);
    void alignItemPositionToRaster(double rasterWidth, double rasterHeight);

    bool isSecondarySelected() const;
    void setSecondarySelected(bool secondarySelected);
    bool isFocusSelected() const;
    void setFocusSelected(bool focusSelected);

    QPointF handlePos(int index);
    void insertHandle(int beforeIndex, const QPointF &pos);
    void deleteHandle(int index);
    void setHandlePos(int index, const QPointF &pos);
    void alignHandleToRaster(int index, double rasterWidth, double rasterHeight);

    virtual void update();

protected:
    virtual void update(const Style *style);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    const Style *adaptedStyle();
    QPointF calcEndPoint(const Uid &end, const Uid &otherEnd, int nearestIntermediatePointIndex);
    QPointF calcEndPoint(const Uid &end, const QPointF &otherEndPos,
                         int nearestIntermediatePointIndex);

    DRelation *m_relation;

protected:
    DiagramSceneModel *m_diagramSceneModel;
    bool m_isSecondarySelected;
    bool m_isFocusSelected;
    ArrowItem *m_arrow;
    QGraphicsSimpleTextItem *m_name;
    StereotypesItem *m_stereotypes;
    PathSelectionItem *m_selectionHandles;
};

} // namespace qmt

#endif // QMT_GRAPHICSRELATIONITEM_H
