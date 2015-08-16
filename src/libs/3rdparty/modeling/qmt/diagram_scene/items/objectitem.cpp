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

#include "objectitem.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/items/stereotypedisplayvisitor.h"
#include "qmt/diagram_scene/parts/alignbuttonsitem.h"
#include "qmt/diagram_scene/parts/rectangularselectionitem.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/stereotypesitem.h"
#include "qmt/infrastructure/contextmenuaction.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mobject.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/style/style.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/style/styledobject.h"
#include "qmt/tasks/diagramscenecontroller.h"
#include "qmt/tasks/ielementtasks.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QCursor>
#include <QMenu>

#include <QDebug>

namespace qmt {

ObjectItem::ObjectItem(DObject *object, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      _object(object),
      _diagram_scene_model(diagram_scene_model),
      _secondary_selected(false),
      _focus_selected(false),
      _stereotype_icon_display(StereotypeIcon::DISPLAY_LABEL),
      _stereotypes(0),
      _stereotype_icon(0),
      _selection_marker(0),
      _horizontal_align_buttons(0),
      _vertical_align_buttons(0)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

ObjectItem::~ObjectItem()
{
}

QRectF ObjectItem::boundingRect() const
{
    return childrenBoundingRect();
}

void ObjectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

QPointF ObjectItem::getPos() const
{
    return _object->getPos();
}

QRectF ObjectItem::getRect() const
{
    return _object->getRect();
}

void ObjectItem::setPosAndRect(const QPointF &original_pos, const QRectF &original_rect, const QPointF &top_left_delta, const QPointF &bottom_right_delta)
{
    QPointF new_pos = original_pos;
    QRectF new_rect = original_rect;
    GeometryUtilities::adjustPosAndRect(&new_pos, &new_rect, top_left_delta, bottom_right_delta, QPointF(0.5, 0.5));
    if (new_pos != _object->getPos() || new_rect != _object->getRect()) {
        _diagram_scene_model->getDiagramController()->startUpdateElement(_object, _diagram_scene_model->getDiagram(), DiagramController::UPDATE_GEOMETRY);
        _object->setPos(new_pos);
        if (new_rect.size() != _object->getRect().size()) {
            _object->setAutoSize(false);
        }
        _object->setRect(new_rect);
        _diagram_scene_model->getDiagramController()->finishUpdateElement(_object, _diagram_scene_model->getDiagram(), false);
    }
}

void ObjectItem::alignItemSizeToRaster(IResizable::Side adjust_horizontal_side, IResizable::Side adjust_vertical_side, double raster_width, double raster_height)
{
    QPointF pos = _object->getPos();
    QRectF rect = _object->getRect();

    double horiz_delta = rect.width() - qRound(rect.width() / raster_width) * raster_width;
    double vert_delta = rect.height() - qRound(rect.height() / raster_height) * raster_height;

    // make sure the new size is at least the minimum size
    QSizeF minimum_size = getMinimumSize();
    while (rect.width() + horiz_delta < minimum_size.width()) {
        horiz_delta += raster_width;
    }
    while (rect.height() + vert_delta < minimum_size.height()) {
        vert_delta += raster_height;
    }

    double left_delta = 0.0;
    double right_delta = 0.0;
    double top_delta = 0.0;
    double bottom_delta = 0.0;

    switch (adjust_horizontal_side) {
    case IResizable::SIDE_NONE:
        break;
    case IResizable::SIDE_LEFT_OR_TOP:
        left_delta = horiz_delta;
        break;
    case IResizable::SIDE_RIGHT_OR_BOTTOM:
        right_delta = -horiz_delta;
        break;
    }

    switch (adjust_vertical_side) {
    case IResizable::SIDE_NONE:
        break;
    case IResizable::SIDE_LEFT_OR_TOP:
        top_delta = vert_delta;
        break;
    case IResizable::SIDE_RIGHT_OR_BOTTOM:
        bottom_delta = -vert_delta;
        break;
    }

    QPointF top_left_delta(left_delta, top_delta);
    QPointF bottom_right_delta(right_delta, bottom_delta);
    setPosAndRect(pos, rect, top_left_delta, bottom_right_delta);
}

void ObjectItem::moveDelta(const QPointF &delta)
{
    _diagram_scene_model->getDiagramController()->startUpdateElement(_object, _diagram_scene_model->getDiagram(), DiagramController::UPDATE_GEOMETRY);
    _object->setPos(_object->getPos() + delta);
    _diagram_scene_model->getDiagramController()->finishUpdateElement(_object, _diagram_scene_model->getDiagram(), false);
}

void ObjectItem::alignItemPositionToRaster(double raster_width, double raster_height)
{
    QPointF pos = _object->getPos();
    QRectF rect = _object->getRect();
    QPointF top_left = pos + rect.topLeft();

    double left_delta = qRound(top_left.x() / raster_width) * raster_width - top_left.x();
    double top_delta = qRound(top_left.y() / raster_height) * raster_height - top_left.y();
    QPointF top_left_delta(left_delta, top_delta);

    setPosAndRect(pos, rect, top_left_delta, top_left_delta);
}

bool ObjectItem::isSecondarySelected() const
{
    return _secondary_selected;
}

void ObjectItem::setSecondarySelected(bool secondary_selected)
{
    if (_secondary_selected != secondary_selected) {
        _secondary_selected = secondary_selected;
        update();
    }
}

bool ObjectItem::isFocusSelected() const
{
    return _focus_selected;
}

void ObjectItem::setFocusSelected(bool focus_selected)
{
    if (_focus_selected != focus_selected) {
        _focus_selected = focus_selected;
        update();
    }
}

QList<ILatchable::Latch> ObjectItem::getHorizontalLatches(ILatchable::Action action, bool grabbed_item) const
{
    Q_UNUSED(grabbed_item);

    QRectF rect = getObject()->getRect();
    rect.translate(getObject()->getPos());
    QList<ILatchable::Latch> result;
    switch (action) {
    case ILatchable::MOVE:
        result << ILatchable::Latch(ILatchable::LEFT, rect.left(), rect.top(), rect.bottom(), QStringLiteral("left"))
               << ILatchable::Latch(ILatchable::HCENTER, rect.center().x(), rect.top(), rect.bottom(), QStringLiteral("center"))
               << ILatchable::Latch(ILatchable::RIGHT, rect.right(), rect.top(), rect.bottom(), QStringLiteral("right"));
        break;
    case ILatchable::RESIZE_LEFT:
        result << ILatchable::Latch(ILatchable::LEFT, rect.left(), rect.top(), rect.bottom(), QStringLiteral("left"));
        break;
    case ILatchable::RESIZE_TOP:
        QMT_CHECK(false);
        break;
    case ILatchable::RESIZE_RIGHT:
        result << ILatchable::Latch(ILatchable::RIGHT, rect.right(), rect.top(), rect.bottom(), QStringLiteral("right"));
        break;
    case ILatchable::RESIZE_BOTTOM:
        QMT_CHECK(false);
        break;
    }
    return result;
}

QList<ILatchable::Latch> ObjectItem::getVerticalLatches(ILatchable::Action action, bool grabbed_item) const
{
    Q_UNUSED(grabbed_item);

    QRectF rect = getObject()->getRect();
    rect.translate(getObject()->getPos());
    QList<ILatchable::Latch> result;
    switch (action) {
    case ILatchable::MOVE:
        result << ILatchable::Latch(ILatchable::TOP, rect.top(), rect.left(), rect.right(), QStringLiteral("top"))
               << ILatchable::Latch(ILatchable::VCENTER, rect.center().y(), rect.left(), rect.right(), QStringLiteral("center"))
               << ILatchable::Latch(ILatchable::BOTTOM, rect.bottom(), rect.left(), rect.right(), QStringLiteral("bottom"));
        break;
    case ILatchable::RESIZE_LEFT:
        QMT_CHECK(false);
        break;
    case ILatchable::RESIZE_TOP:
        result << ILatchable::Latch(ILatchable::TOP, rect.top(), rect.left(), rect.right(), QStringLiteral("top"));
        break;
    case ILatchable::RESIZE_RIGHT:
        QMT_CHECK(false);
        break;
    case ILatchable::RESIZE_BOTTOM:
        result << ILatchable::Latch(ILatchable::BOTTOM, rect.bottom(), rect.left(), rect.right(), QStringLiteral("bottom"));
        break;
    }
    return result;
}

void ObjectItem::align(IAlignable::AlignType align_type, const QString &identifier)
{
    Q_UNUSED(identifier); // avoid warning in release mode

    // subclasses may support other identifiers than the standard ones.
    // but this implementation does not. So assert the names.
    switch (align_type) {
    case IAlignable::ALIGN_LEFT:
        QMT_CHECK(identifier == QStringLiteral("left"));
        _diagram_scene_model->getDiagramSceneController()->alignLeft(_object, _diagram_scene_model->getSelectedElements(), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_RIGHT:
        QMT_CHECK(identifier == QStringLiteral("right"));
        _diagram_scene_model->getDiagramSceneController()->alignRight(_object, _diagram_scene_model->getSelectedElements(), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_TOP:
        QMT_CHECK(identifier == QStringLiteral("top"));
        _diagram_scene_model->getDiagramSceneController()->alignTop(_object, _diagram_scene_model->getSelectedElements(), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_BOTTOM:
        QMT_CHECK(identifier == QStringLiteral("bottom"));
        _diagram_scene_model->getDiagramSceneController()->alignBottom(_object, _diagram_scene_model->getSelectedElements(), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_HCENTER:
        QMT_CHECK(identifier == QStringLiteral("center"));
        _diagram_scene_model->getDiagramSceneController()->alignHCenter(_object, _diagram_scene_model->getSelectedElements(), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_VCENTER:
        QMT_CHECK(identifier == QStringLiteral("center"));
        _diagram_scene_model->getDiagramSceneController()->alignVCenter(_object, _diagram_scene_model->getSelectedElements(), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_WIDTH:
        QMT_CHECK(identifier == QStringLiteral("width"));
        _diagram_scene_model->getDiagramSceneController()->alignWidth(_object, _diagram_scene_model->getSelectedElements(),
                                                                      getMinimumSize(_diagram_scene_model->getSelectedItems()), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_HEIGHT:
        QMT_CHECK(identifier == QStringLiteral("height"));
        _diagram_scene_model->getDiagramSceneController()->alignHeight(_object, _diagram_scene_model->getSelectedElements(),
                                                                       getMinimumSize(_diagram_scene_model->getSelectedItems()), _diagram_scene_model->getDiagram());
        break;
    case IAlignable::ALIGN_SIZE:
        QMT_CHECK(identifier == QStringLiteral("size"));
        _diagram_scene_model->getDiagramSceneController()->alignSize(_object, _diagram_scene_model->getSelectedElements(),
                                                                     getMinimumSize(_diagram_scene_model->getSelectedItems()), _diagram_scene_model->getDiagram());
        break;
    }
}

void ObjectItem::updateStereotypeIconDisplay()
{
    StereotypeDisplayVisitor stereotype_display_visitor;
    stereotype_display_visitor.setModelController(_diagram_scene_model->getDiagramSceneController()->getModelController());
    stereotype_display_visitor.setStereotypeController(_diagram_scene_model->getStereotypeController());
    _object->accept(&stereotype_display_visitor);
    _stereotype_icon_id = stereotype_display_visitor.getStereotypeIconId();
    _shape_icon_id = stereotype_display_visitor.getShapeIconId();
    _stereotype_icon_display = stereotype_display_visitor.getStereotypeIconDisplay();
}

void ObjectItem::updateStereotypes(const QString &stereotype_icon_id, StereotypeIcon::Display stereotype_display, const Style *style)
{
    QList<QString> stereotypes = _object->getStereotypes();
    if (!stereotype_icon_id.isEmpty()
            && (stereotype_display == StereotypeIcon::DISPLAY_DECORATION || stereotype_display == StereotypeIcon::DISPLAY_ICON)) {
        stereotypes = _diagram_scene_model->getStereotypeController()->filterStereotypesByIconId(stereotype_icon_id, stereotypes);
    }
    if (!stereotype_icon_id.isEmpty() && stereotype_display == StereotypeIcon::DISPLAY_DECORATION) {
        if (!_stereotype_icon) {
            _stereotype_icon = new CustomIconItem(_diagram_scene_model, this);
        }
        _stereotype_icon->setStereotypeIconId(stereotype_icon_id);
        _stereotype_icon->setBaseSize(QSizeF(_stereotype_icon->getShapeWidth(), _stereotype_icon->getShapeHeight()));
        _stereotype_icon->setBrush(style->getFillBrush());
        _stereotype_icon->setPen(style->getInnerLinePen());
    } else if (_stereotype_icon) {
        _stereotype_icon->scene()->removeItem(_stereotype_icon);
        delete _stereotype_icon;
        _stereotype_icon = 0;
    }
    if (stereotype_display != StereotypeIcon::DISPLAY_NONE && !stereotypes.isEmpty()) {
        if (!_stereotypes) {
            _stereotypes = new StereotypesItem(this);
        }
        _stereotypes->setFont(style->getSmallFont());
        _stereotypes->setBrush(style->getTextBrush());
        _stereotypes->setStereotypes(stereotypes);
    } else if (_stereotypes) {
        _stereotypes->scene()->removeItem(_stereotypes);
        delete _stereotypes;
        _stereotypes = 0;
    }
}

QSizeF ObjectItem::getStereotypeIconMinimumSize(const StereotypeIcon &stereotype_icon, qreal minimum_width, qreal minimum_height) const
{
    Q_UNUSED(minimum_width);

    qreal width = 0.0;
    qreal height = 0.0;
    if (stereotype_icon.hasMinWidth() && !stereotype_icon.hasMinHeight()) {
        width = stereotype_icon.getMinWidth();
        if (stereotype_icon.getSizeLock() == StereotypeIcon::LOCK_HEIGHT || stereotype_icon.getSizeLock() == StereotypeIcon::LOCK_SIZE) {
            height = stereotype_icon.getMinHeight();
        } else {
            height = width * stereotype_icon.getHeight() / stereotype_icon.getWidth();
        }
    } else if (!stereotype_icon.hasMinWidth() && stereotype_icon.hasMinHeight()) {
        height = stereotype_icon.getMinHeight();
        if (stereotype_icon.getSizeLock() == StereotypeIcon::LOCK_WIDTH || stereotype_icon.getSizeLock() == StereotypeIcon::LOCK_SIZE) {
            width = stereotype_icon.getMinWidth();
        } else {
            width = height * stereotype_icon.getWidth() / stereotype_icon.getHeight();
        }
    } else if (stereotype_icon.hasMinWidth() && stereotype_icon.hasMinHeight()) {
        if (stereotype_icon.getSizeLock() == StereotypeIcon::LOCK_RATIO) {
            width = stereotype_icon.getMinWidth();
            height = width * stereotype_icon.getHeight() / stereotype_icon.getWidth();
            if (height < stereotype_icon.getMinHeight()) {
                height = stereotype_icon.getMinHeight();
                width = height * stereotype_icon.getWidth() / stereotype_icon.getHeight();
                QMT_CHECK(width <= stereotype_icon.getMinWidth());
            }
        } else {
            width = stereotype_icon.getMinWidth();
            height = stereotype_icon.getMinHeight();
        }
    } else {
        height = minimum_height;
        width = height * stereotype_icon.getWidth() / stereotype_icon.getHeight();
    }
    return QSizeF(width, height);
}

void ObjectItem::updateDepth()
{
    setZValue(_object->getDepth());
}

void ObjectItem::updateSelectionMarker(CustomIconItem *custom_icon_item)
{
    if (custom_icon_item) {
        StereotypeIcon stereotype_icon = custom_icon_item->getStereotypeIcon();
        ResizeFlags resize_flags = RESIZE_UNLOCKED;
        switch (stereotype_icon.getSizeLock()) {
        case StereotypeIcon::LOCK_NONE:
            resize_flags = RESIZE_UNLOCKED;
            break;
        case StereotypeIcon::LOCK_WIDTH:
            resize_flags = RESIZE_LOCKED_WIDTH;
            break;
        case StereotypeIcon::LOCK_HEIGHT:
            resize_flags = RESIZE_LOCKED_HEIGHT;
            break;
        case StereotypeIcon::LOCK_SIZE:
            resize_flags = RESIZE_LOCKED_SIZE;
            break;
        case StereotypeIcon::LOCK_RATIO:
            resize_flags = RESIZE_LOCKED_RATIO;
            break;
        }
        updateSelectionMarker(resize_flags);
    } else {
        updateSelectionMarker(RESIZE_UNLOCKED);
    }
}

void ObjectItem::updateSelectionMarker(ResizeFlags resize_flags)
{
    if ((isSelected() || isSecondarySelected()) && resize_flags != RESIZE_LOCKED_SIZE) {
        if (!_selection_marker) {
            _selection_marker = new RectangularSelectionItem(this, this);
        }
        switch (resize_flags) {
        case RESIZE_UNLOCKED:
            _selection_marker->setFreedom(RectangularSelectionItem::FREEDOM_ANY);
            break;
        case RESIZE_LOCKED_SIZE:
            QMT_CHECK(false);
            break;
        case RESIZE_LOCKED_WIDTH:
            _selection_marker->setFreedom(RectangularSelectionItem::FREEDOM_VERTICAL_ONLY);
            break;
        case RESIZE_LOCKED_HEIGHT:
            _selection_marker->setFreedom(RectangularSelectionItem::FREEDOM_HORIZONTAL_ONLY);
            break;
        case RESIZE_LOCKED_RATIO:
            _selection_marker->setFreedom(RectangularSelectionItem::FREEDOM_KEEP_RATIO);
            break;
        }
        _selection_marker->setSecondarySelected(isSelected() ? false : isSecondarySelected());
        _selection_marker->setZValue(SELECTION_MARKER_ZVALUE);
    } else if (_selection_marker) {
        if (_selection_marker->scene()) {
            _selection_marker->scene()->removeItem(_selection_marker);
        }
        delete _selection_marker;
        _selection_marker = 0;
    }
}

void ObjectItem::updateSelectionMarkerGeometry(const QRectF &object_rect)
{
    if (_selection_marker) {
        _selection_marker->setRect(object_rect);
    }
}

void ObjectItem::updateAlignmentButtons()
{
    if (isFocusSelected() && _diagram_scene_model->hasMultiObjectsSelection()) {
        if (!_horizontal_align_buttons && scene()) {
            _horizontal_align_buttons = new AlignButtonsItem(this, 0);
            _horizontal_align_buttons->setZValue(ALIGN_BUTTONS_ZVALUE);
            scene()->addItem(_horizontal_align_buttons);
        }

        if (!_vertical_align_buttons && scene()) {
            _vertical_align_buttons = new AlignButtonsItem(this, 0);
            _vertical_align_buttons->setZValue(ALIGN_BUTTONS_ZVALUE);
            scene()->addItem(_vertical_align_buttons);
        }
    } else {
        if (_horizontal_align_buttons) {
            if (_horizontal_align_buttons->scene()) {
                _horizontal_align_buttons->scene()->removeItem(_horizontal_align_buttons);
            }
            delete _horizontal_align_buttons;
            _horizontal_align_buttons = 0;
        }
        if (_vertical_align_buttons) {
            if (_vertical_align_buttons->scene()) {
                _vertical_align_buttons->scene()->removeItem(_vertical_align_buttons);
            }
            delete _vertical_align_buttons;
            _vertical_align_buttons = 0;
        }
    }
}

void ObjectItem::updateAlignmentButtonsGeometry(const QRectF &object_rect)
{
    if (_horizontal_align_buttons) {
        _horizontal_align_buttons->clear();
        _horizontal_align_buttons->setPos(mapToScene(QPointF(0.0, object_rect.top() - AlignButtonsItem::NORMAL_BUTTON_HEIGHT - AlignButtonsItem::VERTICAL_DISTANCE_TO_OBEJCT)));
        foreach (const ILatchable::Latch &latch, getHorizontalLatches(ILatchable::MOVE, true)) {
            _horizontal_align_buttons->addButton(translateLatchTypeToAlignType(latch._latch_type), latch._identifier, mapFromScene(QPointF(latch._pos, 0.0)).x());
        }
    }
    if (_vertical_align_buttons) {
        _vertical_align_buttons->clear();
        _vertical_align_buttons->setPos(mapToScene(QPointF(object_rect.left() - AlignButtonsItem::NORMAL_BUTTON_WIDTH - AlignButtonsItem::HORIZONTAL_DISTANCE_TO_OBJECT, 0.0)));
        foreach (const ILatchable::Latch &latch, getVerticalLatches(ILatchable::MOVE, true)) {
            _vertical_align_buttons->addButton(translateLatchTypeToAlignType(latch._latch_type), latch._identifier, mapFromScene(QPointF(0.0, latch._pos)).y());
        }
    }
}

IAlignable::AlignType ObjectItem::translateLatchTypeToAlignType(ILatchable::LatchType latch_type)
{
    IAlignable::AlignType align_type = IAlignable::ALIGN_LEFT;
    switch (latch_type) {
    case ILatchable::LEFT:
        align_type = IAlignable::ALIGN_LEFT;
        break;
    case ILatchable::TOP:
        align_type = IAlignable::ALIGN_TOP;
        break;
    case ILatchable::RIGHT:
        align_type = IAlignable::ALIGN_RIGHT;
        break;
    case ILatchable::BOTTOM:
        align_type = IAlignable::ALIGN_BOTTOM;
        break;
    case ILatchable::HCENTER:
        align_type = IAlignable::ALIGN_HCENTER;
        break;
    case ILatchable::VCENTER:
        align_type = IAlignable::ALIGN_VCENTER;
        break;
    case ILatchable::NONE:
        QMT_CHECK(false);
        break;
    }
    return align_type;
}

const Style *ObjectItem::getAdaptedStyle(const QString &stereotype_icon_id)
{
    QList<const DObject *> colliding_objects;
    foreach (const QGraphicsItem *item, _diagram_scene_model->collectCollidingObjectItems(this, DiagramSceneModel::COLLIDING_ITEMS)) {
        if (const ObjectItem *object_item = dynamic_cast<const ObjectItem *>(item)) {
            colliding_objects.append(object_item->getObject());
        }
    }
    QColor base_color;
    if (!stereotype_icon_id.isEmpty()) {
        StereotypeIcon stereotype_icon = _diagram_scene_model->getStereotypeController()->findStereotypeIcon(stereotype_icon_id);
        base_color = stereotype_icon.getBaseColor();
    }
    return _diagram_scene_model->getStyleController()->adaptObjectStyle(
                StyledObject(getObject(),
                             ObjectVisuals(
                                 getObject()->getVisualPrimaryRole(),
                                 getObject()->getVisualSecondaryRole(),
                                 getObject()->isVisualEmphasized(),
                                 base_color,
                                 getObject()->getDepth()),
                             colliding_objects));
}

bool ObjectItem::showContext() const
{
    bool showContext = !_object->getContext().isEmpty();
    if (showContext) {
        // TODO Because of this algorithm adding, moving, removing of one item need to update() all colliding items as well
        QMT_CHECK(getObject()->getModelUid().isValid());
        MObject *mobject = _diagram_scene_model->getDiagramController()->getModelController()->findObject(getObject()->getModelUid());
        QMT_CHECK(mobject);
        MObject *owner = mobject->getOwner();
        if (owner) {
            foreach (QGraphicsItem *item, _diagram_scene_model->collectCollidingObjectItems(this, DiagramSceneModel::COLLIDING_OUTER_ITEMS)) {
                if (ObjectItem *object_item = dynamic_cast<ObjectItem *>(item)) {
                    if (object_item->getObject()->getModelUid().isValid() && object_item->getObject()->getModelUid() == owner->getUid()) {
                        showContext = false;
                        break;
                    }
                }
            }
        }
    }
    return showContext;
}

bool ObjectItem::extendContextMenu(QMenu *menu)
{
    Q_UNUSED(menu);

    return false;
}

bool ObjectItem::handleSelectedContextMenuAction(QAction *action)
{
    Q_UNUSED(action);

    return false;
}

void ObjectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        _diagram_scene_model->selectItem(this, event->modifiers() & Qt::ControlModifier);
    }
    if (event->button() == Qt::LeftButton) {
        _diagram_scene_model->moveSelectedItems(this, QPointF(0.0, 0.0));
    }
}

void ObjectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        _diagram_scene_model->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
    }
}

void ObjectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _diagram_scene_model->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton)) {
            _diagram_scene_model->alignSelectedItemsPositionOnRaster();
        }
    }
}

void ObjectItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        _diagram_scene_model->onDoubleClickedItem(this);
    }
}

void ObjectItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;

    bool add_separator = false;
    if (getDiagramSceneModel()->getDiagramSceneController()->getElementTasks()->hasDiagram(_object, _diagram_scene_model->getDiagram())) {
        menu.addAction(new ContextMenuAction(QObject::tr("Open Diagram"), QStringLiteral("openDiagram"), &menu));
        add_separator = true;
    } else if (getDiagramSceneModel()->getDiagramSceneController()->getElementTasks()->mayCreateDiagram(_object, _diagram_scene_model->getDiagram())) {
        menu.addAction(new ContextMenuAction(QObject::tr("Create Diagram"), QStringLiteral("createDiagram"), &menu));
        add_separator = true;
    }
    if (extendContextMenu(&menu)) {
        add_separator = true;
    }
    if (add_separator) {
        menu.addSeparator();
    }
    menu.addAction(new ContextMenuAction(QObject::tr("Remove"), QStringLiteral("remove"), QKeySequence(QKeySequence::Delete), &menu));
    menu.addAction(new ContextMenuAction(QObject::tr("Delete"), QStringLiteral("delete"), QKeySequence(Qt::CTRL + Qt::Key_D), &menu));
    //menu.addAction(new ContextMenuAction(QObject::tr("Select in Model Tree"), QStringLiteral("selectInModelTree"), &menu));
    QMenu align_menu;
    align_menu.setTitle(QObject::tr("Align Objects"));
    align_menu.addAction(new ContextMenuAction(QObject::tr("Align Left"), QStringLiteral("alignLeft"), &align_menu));
    align_menu.addAction(new ContextMenuAction(QObject::tr("Center Vertically"), QStringLiteral("centerVertically"), &align_menu));
    align_menu.addAction(new ContextMenuAction(QObject::tr("Align Right"), QStringLiteral("alignRight"), &align_menu));
    align_menu.addSeparator();
    align_menu.addAction(new ContextMenuAction(QObject::tr("Align Top"), QStringLiteral("alignTop"), &align_menu));
    align_menu.addAction(new ContextMenuAction(QObject::tr("Center Horizontally"), QStringLiteral("centerHorizontally"), &align_menu));
    align_menu.addAction(new ContextMenuAction(QObject::tr("Align Bottom"), QStringLiteral("alignBottom"), &align_menu));
    align_menu.addSeparator();
    align_menu.addAction(new ContextMenuAction(QObject::tr("Same Width"), QStringLiteral("sameWidth"), &align_menu));
    align_menu.addAction(new ContextMenuAction(QObject::tr("Same Height"), QStringLiteral("sameHeight"), &align_menu));
    align_menu.addAction(new ContextMenuAction(QObject::tr("Same Size"), QStringLiteral("sameSize"), &align_menu));
    align_menu.setEnabled(_diagram_scene_model->hasMultiObjectsSelection());
    menu.addMenu(&align_menu);

    QAction *selected_action = menu.exec(event->screenPos());
    if (selected_action) {
        if (!handleSelectedContextMenuAction(selected_action)) {
            ContextMenuAction *action = dynamic_cast<ContextMenuAction *>(selected_action);
            QMT_CHECK(action);
            if (action->getId() == QStringLiteral("openDiagram")) {
                _diagram_scene_model->getDiagramSceneController()->getElementTasks()->openDiagram(_object, _diagram_scene_model->getDiagram());
            } else if (action->getId() == QStringLiteral("createDiagram")) {
                _diagram_scene_model->getDiagramSceneController()->getElementTasks()->createAndOpenDiagram(_object, _diagram_scene_model->getDiagram());
            } else if (action->getId() == QStringLiteral("remove")) {
                DSelection selection = _diagram_scene_model->getSelectedElements();
                if (selection.isEmpty()) {
                    selection.append(_object->getUid(), _diagram_scene_model->getDiagram()->getUid());
                }
                _diagram_scene_model->getDiagramController()->deleteElements(selection, _diagram_scene_model->getDiagram());
            } else if (action->getId() == QStringLiteral("delete")) {
                DSelection selection = _diagram_scene_model->getSelectedElements();
                if (selection.isEmpty()) {
                    selection.append(_object->getUid(), _diagram_scene_model->getDiagram()->getUid());
                }
                _diagram_scene_model->getDiagramSceneController()->deleteFromDiagram(selection, _diagram_scene_model->getDiagram());
            } else if (action->getId() == QStringLiteral("selectInModelTree")) {
                // TODO implement
            } else if (action->getId() == QStringLiteral("alignLeft")) {
                align(IAlignable::ALIGN_LEFT, QStringLiteral("left"));
            } else if (action->getId() == QStringLiteral("centerVertically")) {
                align(IAlignable::ALIGN_VCENTER, QStringLiteral("center"));
            } else if (action->getId() == QStringLiteral("alignRight")) {
                align(IAlignable::ALIGN_RIGHT, QStringLiteral("right"));
            } else if (action->getId() == QStringLiteral("alignTop")) {
                align(IAlignable::ALIGN_TOP, QStringLiteral("top"));
            } else if (action->getId() == QStringLiteral("centerHorizontally")) {
                align(IAlignable::ALIGN_HCENTER, QStringLiteral("center"));
            } else if (action->getId() == QStringLiteral("alignBottom")) {
                align(IAlignable::ALIGN_BOTTOM, QStringLiteral("bottom"));
            } else if (action->getId() == QStringLiteral("sameWidth")) {
                align(IAlignable::ALIGN_WIDTH, QStringLiteral("width"));
            } else if (action->getId() == QStringLiteral("sameHeight")) {
                align(IAlignable::ALIGN_HEIGHT, QStringLiteral("height"));
            } else if (action->getId() == QStringLiteral("sameSize")) {
                align(IAlignable::ALIGN_SIZE, QStringLiteral("size"));
            }
        }
    }
}

QSizeF ObjectItem::getMinimumSize(const QSet<QGraphicsItem *> &items) const
{
    QSizeF minimum_size(0.0, 0.0);
    foreach (QGraphicsItem *item, items) {
        if (IResizable *resizable = dynamic_cast<IResizable *>(item)) {
            QSizeF size = resizable->getMinimumSize();
            if (size.width() > minimum_size.width()) {
                minimum_size.setWidth(size.width());
            }
            if (size.height() > minimum_size.height()) {
                minimum_size.setHeight(size.height());
            }
        }
    }
    return minimum_size;
}

}
