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

    QPointF handlePos(int index) override;
    void insertHandle(int beforeIndex, const QPointF &pos) override;
    void deleteHandle(int index) override;
    void setHandlePos(int index, const QPointF &pos) override;
    void alignHandleToRaster(int index, double rasterWidth, double rasterHeight) override;

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

    DRelation *m_relation = 0;

protected:
    DiagramSceneModel *m_diagramSceneModel = 0;
    bool m_isSecondarySelected = false;
    bool m_isFocusSelected = false;
    ArrowItem *m_arrow = 0;
    QGraphicsSimpleTextItem *m_name = 0;
    StereotypesItem *m_stereotypes = 0;
    PathSelectionItem *m_selectionHandles = 0;
};

} // namespace qmt

#endif // QMT_GRAPHICSRELATIONITEM_H
