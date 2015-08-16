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

#include "annotationitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/rectangularselectionitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/style/style.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QTextFrame>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QCoreApplication>


namespace qmt {

static const qreal MINIMUM_TEXT_WIDTH = 20.0;
static const qreal CONTENTS_BORDER_VERTICAL = 4.0;
static const qreal CONTENTS_BORDER_HORIZONTAL = 4.0;


class AnnotationItem::AnnotationTextItem :
        public QGraphicsTextItem
{
public:
    AnnotationTextItem(QGraphicsItem *parent)
        : QGraphicsTextItem(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QMT_CHECK(option);

        QStyleOptionGraphicsItem option2(*option);
        option2.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
        QGraphicsTextItem::paint(painter, &option2, widget);
    }
};


AnnotationItem::AnnotationItem(DAnnotation *annotation, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      _annotation(annotation),
      _diagram_scene_model(diagram_scene_model),
      _secondary_selected(false),
      _focus_selected(false),
      _selection_marker(0),
      _no_text_item(0),
      _text_item(0),
      _on_update(false),
      _on_changed(false)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

AnnotationItem::~AnnotationItem()
{
}

QRectF AnnotationItem::boundingRect() const
{
    return childrenBoundingRect();
}

void AnnotationItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void AnnotationItem::update()
{
    QMT_CHECK(!_on_update);
    _on_update = true;

    prepareGeometryChange();

    const Style *style = getAdaptedStyle();

    // text
    if (!_text_item) {
        _text_item = new AnnotationTextItem(this);
        _text_item->setTextInteractionFlags(Qt::TextEditorInteraction);
        _text_item->installSceneEventFilter(this);
        QObject::connect(_text_item->document(), &QTextDocument::contentsChanged, _text_item,
                         [=]() { this->onContentsChanged(); } );
    }
    _text_item->setFont(style->getNormalFont());
    _text_item->setDefaultTextColor(style->getTextBrush().color());
    if (!_on_changed) {
        _text_item->setPlainText(getAnnotation()->getText());
    }

    // item shown if annotation has no text and is not selected
    if (!_no_text_item) {
        _no_text_item = new QGraphicsRectItem(this);
    }
    _no_text_item->setPen(QPen(QBrush(QColor(192, 192, 192)), 1, Qt::DashDotLine));
    _no_text_item->setVisible(!isSelected() && _text_item->document()->isEmpty());

    updateSelectionMarker();

    updateGeometry();

    setZValue(ANNOTATION_ITEMS_ZVALUE);

    _on_update = false;
}

QPointF AnnotationItem::getPos() const
{
    return _annotation->getPos();
}

QRectF AnnotationItem::getRect() const
{
    return _annotation->getRect();
}

QSizeF AnnotationItem::getMinimumSize() const
{
    return calcMinimumGeometry();
}

void AnnotationItem::setPosAndRect(const QPointF &original_pos, const QRectF &original_rect, const QPointF &top_left_delta, const QPointF &bottom_right_delta)
{
    QPointF new_pos = original_pos;
    QRectF new_rect = original_rect;
    GeometryUtilities::adjustPosAndRect(&new_pos, &new_rect, top_left_delta, bottom_right_delta, QPointF(0.0, 0.0));
    if (new_pos != _annotation->getPos() || new_rect != _annotation->getRect()) {
        _diagram_scene_model->getDiagramController()->startUpdateElement(_annotation, _diagram_scene_model->getDiagram(), DiagramController::UPDATE_GEOMETRY);
        _annotation->setPos(new_pos);
        if (new_rect.size() != _annotation->getRect().size()) {
            _annotation->setAutoSize(false);
        }
        _annotation->setRect(new_rect);
        _diagram_scene_model->getDiagramController()->finishUpdateElement(_annotation, _diagram_scene_model->getDiagram(), false);
    }
}

void AnnotationItem::alignItemSizeToRaster(Side adjust_horizontal_side, Side adjust_vertical_side, double raster_width, double raster_height)
{
    Q_UNUSED(adjust_horizontal_side);
    Q_UNUSED(adjust_vertical_side);
    Q_UNUSED(raster_width);
    Q_UNUSED(raster_height);
}

void AnnotationItem::moveDelta(const QPointF &delta)
{
    _diagram_scene_model->getDiagramController()->startUpdateElement(_annotation, _diagram_scene_model->getDiagram(), DiagramController::UPDATE_GEOMETRY);
    _annotation->setPos(_annotation->getPos() + delta);
    _diagram_scene_model->getDiagramController()->finishUpdateElement(_annotation, _diagram_scene_model->getDiagram(), false);
}

void AnnotationItem::alignItemPositionToRaster(double raster_width, double raster_height)
{
    QPointF pos = _annotation->getPos();
    QRectF rect = _annotation->getRect();
    QPointF top_left = pos + rect.topLeft();

    double left_delta = qRound(top_left.x() / raster_width) * raster_width - top_left.x();
    double top_delta = qRound(top_left.y() / raster_height) * raster_height - top_left.y();
    QPointF top_left_delta(left_delta, top_delta);

    setPosAndRect(pos, rect, top_left_delta, top_left_delta);
}

bool AnnotationItem::isSecondarySelected() const
{
    return _secondary_selected;
}

void AnnotationItem::setSecondarySelected(bool secondary_selected)
{
    if (_secondary_selected != secondary_selected) {
        _secondary_selected = secondary_selected;
        update();
    }
}

bool AnnotationItem::isFocusSelected() const
{
    return _focus_selected;
}

void AnnotationItem::setFocusSelected(bool focus_selected)
{
    if (_focus_selected != focus_selected) {
        _focus_selected = focus_selected;
        update();
    }
}

bool AnnotationItem::isEditable() const
{
    return true;
}

void AnnotationItem::edit()
{
    if (_text_item) {
        _text_item->setFocus();
    }
}

void AnnotationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        _diagram_scene_model->selectItem(this, event->modifiers() & Qt::ControlModifier);
    }
    if (event->buttons() & Qt::LeftButton) {
        _diagram_scene_model->moveSelectedItems(this, QPointF(0.0, 0.0));
    }
}

void AnnotationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        _diagram_scene_model->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
    }
}

void AnnotationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _diagram_scene_model->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton)) {
            _diagram_scene_model->alignSelectedItemsPositionOnRaster();
        }
    }
}

void AnnotationItem::updateSelectionMarker()
{
    if (isSelected() || _secondary_selected) {
        if (!_selection_marker) {
            _selection_marker = new RectangularSelectionItem(this, this);
            _selection_marker->setShowBorder(true);
            _selection_marker->setFreedom(RectangularSelectionItem::FREEDOM_HORIZONTAL_ONLY);
        }
        _selection_marker->setSecondarySelected(isSelected() ? false : _secondary_selected);
    } else if (_selection_marker) {
        if (_selection_marker->scene()) {
            _selection_marker->scene()->removeItem(_selection_marker);
        }
        delete _selection_marker;
        _selection_marker = 0;
    }
}

void AnnotationItem::updateSelectionMarkerGeometry(const QRectF &annotation_rect)
{
    if (_selection_marker) {
        _selection_marker->setRect(annotation_rect);
    }
}

const Style *AnnotationItem::getAdaptedStyle()
{
    return _diagram_scene_model->getStyleController()->adaptAnnotationStyle(_annotation);
}

bool AnnotationItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (watched == _text_item) {
        if (event->type() == QEvent::FocusIn) {
            scene()->clearSelection();
            setSelected(true);
        }
    }
    return false;
}

void AnnotationItem::onContentsChanged()
{
    QMT_CHECK(!_on_changed);
    _on_changed = true;

    if (!_on_update) {
        QString plain_text = _text_item->toPlainText();
        if (_annotation->getText() != plain_text) {
            _diagram_scene_model->getDiagramController()->startUpdateElement(_annotation, _diagram_scene_model->getDiagram(), DiagramController::UPDATE_MINOR);
            _annotation->setText(plain_text);
            _diagram_scene_model->getDiagramController()->finishUpdateElement(_annotation, _diagram_scene_model->getDiagram(), false);
        }
    }

    _on_changed = false;
}

QSizeF AnnotationItem::calcMinimumGeometry() const
{
    qreal width = MINIMUM_TEXT_WIDTH + 2 * CONTENTS_BORDER_HORIZONTAL;
    qreal height = 0.0; // irrelevant; cannot be modified by user and will always be overwritten

    if (getAnnotation()->hasAutoSize()) {
        if (_text_item) {
            _text_item->setTextWidth(-1);
            QSizeF text_size = _text_item->document()->size();
            width = text_size.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
        }
    }
    return QSizeF(width, height);
}

void AnnotationItem::updateGeometry()
{
    prepareGeometryChange();

    QSizeF geometry = calcMinimumGeometry();
    qreal width = geometry.width();
    qreal height = geometry.height();

    if (getAnnotation()->hasAutoSize()) {
        if (_text_item) {
            _text_item->setTextWidth(-1);
            QSizeF text_size = _text_item->document()->size();
            width = text_size.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
            height = text_size.height() + 2 * CONTENTS_BORDER_VERTICAL;
        }
    } else {
        QRectF rect = getAnnotation()->getRect();
        width = rect.width();
        if (_text_item) {
            _text_item->setTextWidth(width - 2 * CONTENTS_BORDER_HORIZONTAL);
            height = _text_item->document()->size().height() + 2 * CONTENTS_BORDER_VERTICAL;
        }
    }

    // update sizes and positions
    double left = 0.0;
    double top = 0.0;

    setPos(getAnnotation()->getPos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    getAnnotation()->setRect(rect);

    if (_no_text_item) {
        _no_text_item->setRect(rect);
    }

    if (_text_item) {
        _text_item->setPos(left + CONTENTS_BORDER_HORIZONTAL, top + CONTENTS_BORDER_VERTICAL);
    }

    updateSelectionMarkerGeometry(rect);
}

}
