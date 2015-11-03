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

#ifndef QMT_OBJECTITEM_H
#define QMT_OBJECTITEM_H

#include <QGraphicsItem>

#include "qmt/diagram_scene/capabilities/intersectionable.h"
#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/selectable.h"
#include "qmt/diagram_scene/capabilities/latchable.h"
#include "qmt/diagram_scene/capabilities/alignable.h"

#include "qmt/stereotype/stereotypeicon.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE


namespace qmt {

class DObject;
class DiagramSceneModel;
class StereotypesItem;
class CustomIconItem;
class RectangularSelectionItem;
class AlignButtonsItem;
class Style;


// typical z-values for graphic items
static const int SHAPE_ZVALUE = -100; // the filled background of the shape
static const int SHAPE_DETAILS_ZVALUE = -90; // any details to the shape (e.g. extra lines in shape)
static const int SELECTION_MARKER_ZVALUE = 100;

class ObjectItem :
        public QGraphicsItem,
        public IIntersectionable,
        public IResizable,
        public IMoveable,
        public ISelectable,
        public ILatchable,
        public IAlignable
{
protected:

    enum ResizeFlags {
        RESIZE_UNLOCKED,
        RESIZE_LOCKED_SIZE,
        RESIZE_LOCKED_WIDTH,
        RESIZE_LOCKED_HEIGHT,
        RESIZE_LOCKED_RATIO
    };

public:

    ObjectItem(DObject *object, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent = 0);

    ~ObjectItem();

public:

    DObject *getObject() const { return m_object; }

    DiagramSceneModel *getDiagramSceneModel() const { return m_diagramSceneModel; }

public:

    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public:

    virtual void update() = 0;

public:

    bool intersectShapeWithLine(const QLineF &line, QPointF *intersection_point, QLineF *intersection_line) const = 0;

public:

    QPointF getPos() const;

    QRectF getRect() const;

    QSizeF getMinimumSize() const = 0;

    void setPosAndRect(const QPointF &original_pos, const QRectF &original_rect, const QPointF &top_left_delta, const QPointF &bottom_right_delta);

    void alignItemSizeToRaster(Side adjust_horizontal_side, Side adjust_vertical_side, double raster_width, double raster_height);

public:

    void moveDelta(const QPointF &delta);

    void alignItemPositionToRaster(double raster_width, double raster_height);

public:

    bool isSecondarySelected() const;

    void setSecondarySelected(bool secondary_selected);

    bool isFocusSelected() const;

    void setFocusSelected(bool focus_selected);

public:

    QList<Latch> getHorizontalLatches(Action action, bool grabbed_item) const;

    QList<Latch> getVerticalLatches(Action action, bool grabbed_item) const;

public:

    void align(AlignType align_type, const QString &identifier);

protected:

    void updateStereotypeIconDisplay();

    QString getStereotypeIconId() const { return m_stereotypeIconId; }

    QString getShapeIconId() const { return m_shapeIconId; }

    StereotypeIcon::Display getStereotypeIconDisplay() const { return m_stereotypeIconDisplay; }

    void updateStereotypes(const QString &stereotype_icon_id, StereotypeIcon::Display stereotype_display, const Style *style);

    StereotypesItem *getStereotypesItem() const { return m_stereotypes; }

    CustomIconItem *getStereotypeIconItem() const { return m_stereotypeIcon; }

    QSizeF getStereotypeIconMinimumSize(const StereotypeIcon &stereotype_icon, qreal minimum_width, qreal minimum_height) const;

    void updateDepth();

    void updateSelectionMarker(CustomIconItem *custom_icon_item);

    void updateSelectionMarker(ResizeFlags resize_flags);

    void updateSelectionMarkerGeometry(const QRectF &object_rect);

    void updateAlignmentButtons();

    void updateAlignmentButtonsGeometry(const QRectF &object_rect);

    IAlignable::AlignType translateLatchTypeToAlignType(ILatchable::LatchType latch_type);

    const Style *getAdaptedStyle(const QString &stereotype_icon_id);

    bool showContext() const;

    virtual bool extendContextMenu(QMenu *menu);

    virtual bool handleSelectedContextMenuAction(QAction *action);

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

private:

    QSizeF getMinimumSize(const QSet<QGraphicsItem *> &items) const;

private:

    DObject *m_object;

    DiagramSceneModel *m_diagramSceneModel;

    bool m_secondarySelected;

    bool m_focusSelected;

    QString m_stereotypeIconId;

    QString m_shapeIconId;

    StereotypeIcon::Display m_stereotypeIconDisplay;

    StereotypesItem *m_stereotypes;

    CustomIconItem *m_stereotypeIcon;

    RectangularSelectionItem *m_selectionMarker;

    AlignButtonsItem *m_horizontalAlignButtons;

    AlignButtonsItem *m_verticalAlignButtons;

};

}

#endif // QMT_OBJECTITEM_H
