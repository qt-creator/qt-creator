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

#ifndef QMT_BOUNDARYITEM_H
#define QMT_BOUNDARYITEM_H

#include <QGraphicsItem>

#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/selectable.h"
#include "qmt/diagram_scene/capabilities/editable.h"

namespace qmt {

class DBoundary;
class DiagramSceneModel;
class RectangularSelectionItem;
class Style;

class BoundaryItem :
        public QGraphicsItem,
        public IResizable,
        public IMoveable,
        public ISelectable,
        public IEditable
{
    class BoundaryTextItem;

public:
    BoundaryItem(DBoundary *boundary, DiagramSceneModel *diagramSceneModel,
                 QGraphicsItem *parent = 0);
    ~BoundaryItem() override;

    DBoundary *boundary() const { return m_boundary; }
    DiagramSceneModel *diagramSceneModel() const { return m_diagramSceneModel; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void update();

    QPointF pos() const override;
    QRectF rect() const override;
    QSizeF minimumSize() const override;
    void setPosAndRect(const QPointF &originalPos, const QRectF &originalRect,
                       const QPointF &topLeftDelta, const QPointF &bottomRightDelta) override;
    void alignItemSizeToRaster(Side adjustHorizontalSide, Side adjustVerticalSide,
                               double rasterWidth, double rasterHeight) override;

    void moveDelta(const QPointF &delta) override;
    void alignItemPositionToRaster(double rasterWidth, double rasterHeight) override;

    bool isSecondarySelected() const override;
    void setSecondarySelected(bool secondarySelected) override;
    bool isFocusSelected() const override;
    void setFocusSelected(bool focusSelected) override;

    bool isEditable() const override;
    void edit() override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void updateSelectionMarker();
    void updateSelectionMarkerGeometry(const QRectF &boundaryRect);
    const Style *adaptedStyle();
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    void onContentsChanged();

    QSizeF calcMinimumGeometry() const;
    void updateGeometry();

    DBoundary *m_boundary = 0;
    DiagramSceneModel *m_diagramSceneModel = 0;
    bool m_isSecondarySelected = false;
    bool m_isFocusSelected = false;
    RectangularSelectionItem *m_selectionMarker = 0;
    QGraphicsRectItem *m_borderItem = 0;
    QGraphicsRectItem *m_noTextItem = 0;
    BoundaryTextItem *m_textItem = 0;
    bool m_isUpdating = false;
    bool m_isChanged = false;
};

} // namespace qmt

#endif // QMT_BOUNDARYITEM_H
