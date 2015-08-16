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

#include "diagramscenemodel.h"

#include "diagramgraphicsscene.h"
#include "diagramsceneconstants.h"
#include "diagramscenemodelitemvisitors.h"
#include "latchcontroller.h"
#include "capabilities/moveable.h"
#include "capabilities/resizable.h"
#include "capabilities/selectable.h"
#include "capabilities/editable.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/tasks/diagramscenecontroller.h"
#include "qmt/tasks/ielementtasks.h"

#include <QSet>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <QBuffer>
#include <QPdfWriter>

#ifdef USE_SVG_CLIPBOARD
#include <QtSvg/QSvgGenerator>
#endif

#ifdef USE_EMF_CLIPBOARD
// TODO implement emf clipboard
#endif

#include <QDebug>


namespace qmt {

class DiagramSceneModel::OriginItem :
        public QGraphicsItem
{
public:
    OriginItem(QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
    {
    }

    QRectF boundingRect() const
    {
        return QRectF(0.0, 0.0, 20.0, 20.0);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        QPen pen(QBrush(Qt::gray), 1.0, Qt::DotLine);
        painter->setPen(pen);
        painter->drawLine(QLineF(0.0, 0.0, 20.0, 0.0));
        painter->drawLine(QLineF(0.0, 0.0, 0.0, 20.0));
    }
};


DiagramSceneModel::DiagramSceneModel(QObject *parent)
    : QObject(parent),
      _diagram_controller(0),
      _diagram_scene_controller(0),
      _style_controller(0),
      _stereotype_controller(0),
      _diagram(0),
      _graphics_scene(new DiagramGraphicsScene(this, this)),
      _latch_controller(new LatchController(this)),
      _busy(NOT_BUSY),
      _origin_item(new OriginItem()),
      _focus_item(0)
{
    _latch_controller->setDiagramSceneModel(this);
    connect(_graphics_scene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));

    // add one item at origin to force scene rect to include origin always
    _graphics_scene->addItem(_origin_item);

    _latch_controller->addToGraphicsScene(_graphics_scene);

}

DiagramSceneModel::~DiagramSceneModel()
{
    QMT_CHECK(_busy == NOT_BUSY);
    _latch_controller->removeFromGraphicsScene(_graphics_scene);
    disconnect();
    if (_diagram_controller) {
        disconnect(_diagram_controller, 0, this, 0);
    }
}

void DiagramSceneModel::setDiagramController(DiagramController *diagram_controller)
{
    if (_diagram_controller == diagram_controller) {
        return;
    }
    if (_diagram_controller) {
        disconnect(_diagram_controller, 0, this, 0);
        _diagram_controller = 0;
    }
    _diagram_controller = diagram_controller;
    if (diagram_controller) {
        connect(_diagram_controller, SIGNAL(beginResetAllDiagrams()), this, SLOT(onBeginResetAllDiagrams()));
        connect(_diagram_controller, SIGNAL(endResetAllDiagrams()), this, SLOT(onEndResetAllDiagrams()));
        connect(_diagram_controller, SIGNAL(beginResetDiagram(const MDiagram*)), this, SLOT(onBeginResetDiagram(const MDiagram*)));
        connect(_diagram_controller, SIGNAL(endResetDiagram(const MDiagram*)), this, SLOT(onEndResetDiagram(const MDiagram*)));
        connect(_diagram_controller, SIGNAL(beginUpdateElement(int,const MDiagram*)), this, SLOT(onBeginUpdateElement(int,const MDiagram*)));
        connect(_diagram_controller, SIGNAL(endUpdateElement(int,const MDiagram*)), this, SLOT(onEndUpdateElement(int,const MDiagram*)));
        connect(_diagram_controller, SIGNAL(beginInsertElement(int,const MDiagram*)), this, SLOT(onBeginInsertElement(int,const MDiagram*)));
        connect(_diagram_controller, SIGNAL(endInsertElement(int,const MDiagram*)), this, SLOT(onEndInsertElement(int,const MDiagram*)));
        connect(_diagram_controller, SIGNAL(beginRemoveElement(int,const MDiagram*)), this, SLOT(onBeginRemoveElement(int,const MDiagram*)));
        connect(_diagram_controller, SIGNAL(endRemoveElement(int,const MDiagram*)), this, SLOT(onEndRemoveElement(int,const MDiagram*)));
    }
}

void DiagramSceneModel::setDiagramSceneController(DiagramSceneController *diagram_scene_controller)
{
    _diagram_scene_controller = diagram_scene_controller;
}

void DiagramSceneModel::setStyleController(StyleController *style_controller)
{
    _style_controller = style_controller;
}

void DiagramSceneModel::setStereotypeController(StereotypeController *stereotype_controller)
{
    _stereotype_controller = stereotype_controller;
}

void DiagramSceneModel::setDiagram(MDiagram *diagram)
{
    if (_diagram != diagram) {
        onBeginResetDiagram(diagram);
        _diagram = diagram;
        onEndResetDiagram(diagram);
    }
}

QGraphicsScene *DiagramSceneModel::getGraphicsScene() const
{
    return _graphics_scene;
}

bool DiagramSceneModel::hasSelection() const
{
    return !_graphics_scene->selectedItems().isEmpty();
}

bool DiagramSceneModel::hasMultiObjectsSelection() const
{
    int count = 0;
    foreach (QGraphicsItem *item, _graphics_scene->selectedItems()) {
        DElement *element = _item_to_element_map.value(item);
        QMT_CHECK(element);
        if (dynamic_cast<DObject *>(element) != 0) {
            ++count;
            if (count > 1) {
                return true;
            }
        }
    }
    return false;
}

DSelection DiagramSceneModel::getSelectedElements() const
{
    DSelection selection;
    foreach (QGraphicsItem *item, _graphics_scene->selectedItems()) {
        DElement *element = _item_to_element_map.value(item);
        QMT_CHECK(element);
        selection.append(element->getUid(), _diagram->getUid());
    }
    return selection;
}

DElement *DiagramSceneModel::findTopmostElement(const QPointF &scene_pos) const
{
    // fetch affected items from scene in correct drawing order to find topmost element
    QList<QGraphicsItem *> items = _graphics_scene->items(scene_pos);
    foreach (QGraphicsItem *item, items) {
        if (_graphics_items.contains(item)) {
            return _item_to_element_map.value(item);
        }
    }
    return 0;
}

QGraphicsItem *DiagramSceneModel::getGraphicsItem(DElement *element) const
{
    return _element_to_item_map.value(element);
}

QGraphicsItem *DiagramSceneModel::getGraphicsItem(const Uid &uid) const
{
    return _element_to_item_map.value(_diagram_controller->findElement(uid, _diagram));
}

bool DiagramSceneModel::isSelectedItem(QGraphicsItem *item) const
{
    return _selected_items.contains(item);
}

DElement *DiagramSceneModel::getElement(QGraphicsItem *item) const
{
    return _item_to_element_map.value(item);
}

bool DiagramSceneModel::isElementEditable(const DElement *element) const
{
    IEditable *editable = dynamic_cast<IEditable *>(_element_to_item_map.value(element));
    return editable != 0 && editable->isEditable();
}

void DiagramSceneModel::selectAllElements()
{
    foreach (QGraphicsItem *item, _graphics_items) {
        item->setSelected(true);
    }
}

void DiagramSceneModel::selectElement(DElement *element)
{
    QGraphicsItem *select_item = _element_to_item_map.value(element);
    foreach (QGraphicsItem *item, _selected_items) {
        if (item != select_item) {
            item->setSelected(false);
        }
    }
    if (select_item) {
        select_item->setSelected(true);
    }
}

void DiagramSceneModel::editElement(DElement *element)
{
    IEditable *editable = dynamic_cast<IEditable *>(_element_to_item_map.value(element));
    if (editable != 0 && editable->isEditable()) {
        editable->edit();
    }
}

void DiagramSceneModel::copyToClipboard()
{
    QMimeData *mime_data = new QMimeData();

    // Selections would also render to the clipboard
    _graphics_scene->clearSelection();
    removeExtraSceneItems();

    QRectF scene_bounding_rect = _graphics_scene->itemsBoundingRect();

    {
        // Create the image with the size of the shrunk scene
        const int scale_factor = 4;
        const int border = 4;
        const int base_dpi = 75;
        const int dots_per_meter = 10000 * base_dpi / 254;
        QSize image_size = scene_bounding_rect.size().toSize();
        image_size += QSize(2 * border, 2 * border);
        image_size *= scale_factor;
        QImage image(image_size, QImage::Format_ARGB32);
        image.setDotsPerMeterX(dots_per_meter * scale_factor);
        image.setDotsPerMeterY(dots_per_meter * scale_factor);
        image.fill(Qt::white);
        QPainter painter;
        painter.begin(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        _graphics_scene->render(&painter,
                                QRectF(border, border,
                                       painter.device()->width() - 2 * border, painter.device()->height() - 2 * border),
                                scene_bounding_rect);
        painter.end();
        mime_data->setImageData(image);
    }

#ifdef USE_PDF_CLIPBOARD
    {
        const double scale_factor = 1.0;
        const double border = 5;
        const double base_dpi = 100;
        const double dots_per_mm = 25.4 / base_dpi;

        QBuffer pdf_buffer;
        pdf_buffer.open(QIODevice::WriteOnly);

        QPdfWriter pdf_writer(&pdf_buffer);
        QSizeF page_size = scene_bounding_rect.size();
        page_size += QSizeF(2.0 * border, 2.0 * border);
        page_size *= scale_factor;
        pdf_writer.setPageSize(QPdfWriter::Custom);
        pdf_writer.setPageSizeMM(page_size * dots_per_mm);

        QPainter pdf_painter;
        pdf_painter.begin(&pdf_writer);
        _graphics_scene->render(&pdf_painter,
                                QRectF(border, border,
                                       pdf_painter.device()->width() - 2 * border, pdf_painter.device()->height() - 2 * border),
                                scene_bounding_rect);
        pdf_painter.end();
        pdf_buffer.close();
        mime_data->setData(QStringLiteral("application/pdf"), pdf_buffer.buffer());
    }
#endif

#ifdef USE_SVG_CLIPBOARD
    {
        QBuffer svg_buffer;
        QSvgGenerator svg_generator;
        svg_generator.setOutputDevice(&svg_buffer);
        QSize svg_scene_size = scene_bounding_rect.size().toSize();
        svg_generator.setSize(svg_scene_size);
        svg_generator.setViewBox(QRect(QPoint(0,0), svg_scene_size));
        QPainter svg_painter;
        svg_painter.begin(&svg_generator);
        svg_painter.setRenderHint(QPainter::Antialiasing);
        _graphics_scene->render(&svg_painter,
                                QRectF(border, border,
                                       painter.device()->width() - 2 * border, painter.device()->height() - 2 * border),
                                scene_bounding_rect);
        svg_painter.end();
        mime_data->setData(QStringLiteral("image/svg+xml"), svg_buffer.buffer());
    }
#endif

#ifdef USE_EMF_CLIPBOARD
    // TODO implement emf clipboard
#endif

    QApplication::clipboard()->setMimeData(mime_data, QClipboard::Clipboard);

    addExtraSceneItems();
}

bool DiagramSceneModel::exportPng(const QString &file_name)
{
    removeExtraSceneItems();

    QRectF scene_bounding_rect = _graphics_scene->itemsBoundingRect();

    // Create the image with the size of the shrunk scene
    const int scale_factor = 1;
    const int border = 5;
    const int base_dpi = 75;
    const int dots_per_meter = 10000 * base_dpi / 254;

    QSize image_size = scene_bounding_rect.size().toSize();
    image_size += QSize(2 * border, 2 * border);
    image_size *= scale_factor;

    QImage image(image_size, QImage::Format_ARGB32);
    image.setDotsPerMeterX(dots_per_meter * scale_factor);
    image.setDotsPerMeterY(dots_per_meter * scale_factor);
    image.fill(Qt::white);

    QPainter painter;
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    _graphics_scene->render(&painter,
                            QRectF(border, border,
                                   painter.device()->width() - 2 * border, painter.device()->height() - 2 * border),
                            scene_bounding_rect);
    painter.end();

    bool success = image.save(file_name);

    addExtraSceneItems();

    return success;
}

void DiagramSceneModel::exportPdf(const QString &file_name)
{
    removeExtraSceneItems();

    QRectF scene_bounding_rect = _graphics_scene->itemsBoundingRect();

    const double scale_factor = 1.0;
    const double border = 5;
    const double base_dpi = 100;
    const double dots_per_mm = 25.4 / base_dpi;

    QSizeF page_size = scene_bounding_rect.size();
    page_size += QSizeF(2.0 * border, 2.0 * border);
    page_size *= scale_factor;

    QPdfWriter pdf_writer(file_name);
    pdf_writer.setPageSize(QPdfWriter::Custom);
    pdf_writer.setPageSizeMM(page_size * dots_per_mm);

    QPainter pdf_painter;
    pdf_painter.begin(&pdf_writer);
    _graphics_scene->render(&pdf_painter,
                            QRectF(border, border,
                                   pdf_painter.device()->width() - 2 * border, pdf_painter.device()->height() - 2 * border),
                            scene_bounding_rect);
    pdf_painter.end();

    addExtraSceneItems();
}

void DiagramSceneModel::selectItem(QGraphicsItem *item, bool multi_select)
{
    if (!multi_select) {
        if (!item->isSelected()) {
            foreach (QGraphicsItem *selected_item, _selected_items) {
                if (selected_item != item) {
                    selected_item->setSelected(false);
                }
            }
            item->setSelected(true);
        }
    } else {
        item->setSelected(!item->isSelected());
    }
}

void DiagramSceneModel::moveSelectedItems(QGraphicsItem *grabbed_item, const QPointF &delta)
{
    Q_UNUSED(grabbed_item);

    if (delta != QPointF(0.0, 0.0)) {
        foreach (QGraphicsItem *item, _selected_items) {
            if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
                moveable->moveDelta(delta);
            }
        }
        foreach (QGraphicsItem *item, _secondary_selected_items) {
            if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
                moveable->moveDelta(delta);
            }
        }
    }
}

void DiagramSceneModel::alignSelectedItemsPositionOnRaster()
{
    foreach (QGraphicsItem *item, _selected_items) {
        if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
            moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
        }
    }
    foreach (QGraphicsItem *item, _secondary_selected_items) {
        if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
            moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
        }
    }
}

void DiagramSceneModel::onDoubleClickedItem(QGraphicsItem *item)
{
    DElement *element = _item_to_element_map.value(item);
    if (item) {
        _diagram_scene_controller->getElementTasks()->openElement(element, _diagram);
    }
}

QList<QGraphicsItem *> DiagramSceneModel::collectCollidingObjectItems(const QGraphicsItem *item, CollidingMode colliding_mode) const
{
    QList<QGraphicsItem *> colliding_items;

    const IResizable *resizable = dynamic_cast<const IResizable *>(item);
    if (!resizable) {
        return colliding_items;
    }
    QRectF rect = resizable->getRect();
    rect.translate(resizable->getPos());

    switch (colliding_mode) {
    case COLLIDING_INNER_ITEMS:
        foreach (QGraphicsItem *candidate, _graphics_items) {
            if (const IResizable *candidate_resizable = dynamic_cast<const IResizable *>(candidate)) {
                QRectF candidate_rect = candidate_resizable->getRect();
                candidate_rect.translate(candidate_resizable->getPos());
                if (candidate_rect.left() >= rect.left() && candidate_rect.right() <= rect.right()
                        && candidate_rect.top() >= rect.top() && candidate_rect.bottom() <= rect.bottom()) {
                    colliding_items.append(candidate);
                }
            }
        }
        break;
    case COLLIDING_ITEMS:
        foreach (QGraphicsItem *candidate, _graphics_items) {
            if (const IResizable *candidate_resizable = dynamic_cast<const IResizable *>(candidate)) {
                QRectF candidate_rect = candidate_resizable->getRect();
                candidate_rect.translate(candidate_resizable->getPos());
                if (candidate_rect.left() <= rect.right() && candidate_rect.right() >= rect.left()
                        && candidate_rect.top() <= rect.bottom() && candidate_rect.bottom() >= rect.top()) {
                    colliding_items.append(candidate);
                }
            }
        }
        break;
    case COLLIDING_OUTER_ITEMS:
        foreach (QGraphicsItem *candidate, _graphics_items) {
            if (const IResizable *candidate_resizable = dynamic_cast<const IResizable *>(candidate)) {
                QRectF candidate_rect = candidate_resizable->getRect();
                candidate_rect.translate(candidate_resizable->getPos());
                if (candidate_rect.left() <= rect.left() && candidate_rect.right() >= rect.right()
                        && candidate_rect.top() <= rect.top() && candidate_rect.bottom() >= rect.bottom()) {
                    colliding_items.append(candidate);
                }
            }
        }
        break;
    default:
        QMT_CHECK(false);
    }
    return colliding_items;
}

void DiagramSceneModel::sceneActivated()
{
    emit diagramSceneActivated(_diagram);
}

void DiagramSceneModel::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    updateFocusItem(QSet<QGraphicsItem *>::fromList(_graphics_scene->selectedItems()));
    _latch_controller->mousePressEventLatching(event);
    mousePressEventReparenting(event);
}

void DiagramSceneModel::mousePressEventReparenting(QGraphicsSceneMouseEvent *event)
{
    // TODO add keyboard event handler to change cursor also on modifier change without move
    mouseMoveEventReparenting(event);
}

void DiagramSceneModel::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    _latch_controller->mouseMoveEventLatching(event);
    mouseMoveEventReparenting(event);
}

void DiagramSceneModel::mouseMoveEventReparenting(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::AltModifier) {
        // TODO show move cursor only if elements can be moved to underlaying element
        foreach (QGraphicsView *view, _graphics_scene->views()) {
            // TODO find a better cursor that signals "move to this package"
            view->setCursor(QCursor(Qt::OpenHandCursor));
        }
    } else {
        foreach (QGraphicsView *view, _graphics_scene->views()) {
            view->unsetCursor();
        }
    }
}

void DiagramSceneModel::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    _latch_controller->mouseReleaseEventLatching(event);
    mouseReleaseEventReparenting(event);
}

void DiagramSceneModel::mouseReleaseEventReparenting(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::AltModifier) {
        ModelController *model_controller = getDiagramController()->getModelController();
        MPackage *new_owner = 0;
        QSet<QGraphicsItem *> selected_item_set = _graphics_scene->selectedItems().toSet();
        QList<QGraphicsItem *> items_under_mouse = _graphics_scene->items(event->scenePos());
        foreach (QGraphicsItem *item, items_under_mouse) {
            if (!selected_item_set.contains(item)) {
                // the item may be any graphics item not matching to a DElement
                DElement *element = _item_to_element_map.value(item);
                if (element && element->getModelUid().isValid()) {
                    new_owner = model_controller->findElement<MPackage>(element->getModelUid());
                }
            }
            if (new_owner) {
                break;
            }
        }
        if (new_owner) {
            foreach (QGraphicsItem *item, _graphics_scene->selectedItems()) {
                DElement *element = _item_to_element_map.value(item);
                QMT_CHECK(element);
                if (element->getModelUid().isValid()) {
                    MObject *model_object = model_controller->findObject(element->getModelUid());
                    if (model_object) {
                        if (new_owner != model_object->getOwner()) {
                            model_controller->moveObject(new_owner, model_object);
                        }
                    }
                }
            }
        }
    }
    foreach (QGraphicsView *view, _graphics_scene->views()) {
        view->unsetCursor();
    }
}

void DiagramSceneModel::onBeginResetAllDiagrams()
{
    onBeginResetDiagram(_diagram);
}

void DiagramSceneModel::onEndResetAllDiagrams()
{
    onEndResetDiagram(_diagram);
}

void DiagramSceneModel::onBeginResetDiagram(const MDiagram *diagram)
{
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = RESET_DIAGRAM;
    if (diagram == _diagram) {
        clearGraphicsScene();
    }
}

void DiagramSceneModel::onEndResetDiagram(const MDiagram *diagram)
{
    QMT_CHECK(_busy == RESET_DIAGRAM);
    if (diagram == _diagram) {
        QMT_CHECK(_graphics_items.size() == 0);
        // create all items and update graphics item from element initially
        foreach (DElement *element, diagram->getDiagramElements()) {
            QGraphicsItem *item = createGraphicsItem(element);
            _graphics_items.append(item);
            updateGraphicsItem(item, element);
        }
        // invalidate scene
        _graphics_scene->invalidate();
        // update graphics items again so every item gets a correct list of colliding items
        foreach (DElement *element, diagram->getDiagramElements()) {
            updateGraphicsItem(_element_to_item_map.value(element), element);
        }
    }
    _busy = NOT_BUSY;
}

void DiagramSceneModel::onBeginUpdateElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = UPDATE_ELEMENT;

}

void DiagramSceneModel::onEndUpdateElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(_busy == UPDATE_ELEMENT);
    if (diagram == _diagram) {
        QGraphicsItem *item = _graphics_items.at(row);
        updateGraphicsItem(item, diagram->getDiagramElements().at(row));
    }
    _busy = NOT_BUSY;
}

void DiagramSceneModel::onBeginInsertElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(_busy == NOT_BUSY);
    _busy = INSERT_ELEMENT;
}

void DiagramSceneModel::onEndInsertElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(_busy == INSERT_ELEMENT);
    QGraphicsItem *item = 0;
    if (diagram == _diagram) {
        DElement *element = diagram->getDiagramElements().at(row);
        item = createGraphicsItem(element);
        _graphics_items.insert(row, item);
        updateGraphicsItem(item, element);
        _graphics_scene->invalidate();
        updateGraphicsItem(item, element);
    }
    _busy = NOT_BUSY;
}

void DiagramSceneModel::onBeginRemoveElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(_busy == NOT_BUSY);
    if (diagram == _diagram) {
        QGraphicsItem *item = _graphics_items.takeAt(row);
        deleteGraphicsItem(item, diagram->getDiagramElements().at(row));
    }
    _busy = REMOVE_ELEMENT;
}

void DiagramSceneModel::onEndRemoveElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(_busy == REMOVE_ELEMENT);
    _busy = NOT_BUSY;
}

void DiagramSceneModel::onSelectionChanged()
{
    bool selection_changed = false;

    // collect and update all primary selected items (selected by user)
    QSet<QGraphicsItem *> new_selected_items = QSet<QGraphicsItem *>::fromList(_graphics_scene->selectedItems());
    updateFocusItem(new_selected_items);
    foreach (QGraphicsItem *item, _selected_items) {
        if (!new_selected_items.contains(item)) {
            DElement *element = _item_to_element_map.value(item);
            updateGraphicsItem(item, element);
            selection_changed = true;
        }
    }
    foreach (QGraphicsItem *item, new_selected_items) {
        if (!_selected_items.contains(item)) {
            DElement *element = _item_to_element_map.value(item);
            updateGraphicsItem(item, element);
            selection_changed = true;
        }
    }
    _selected_items = new_selected_items;

    // collect and update all secondary selected items
    QSet<QGraphicsItem *> new_secondary_selected_items;

    // select all contained objects secondarily
    foreach (QGraphicsItem *selected_item, _selected_items) {
        foreach (QGraphicsItem *item, collectCollidingObjectItems(selected_item, COLLIDING_INNER_ITEMS)) {
            if (!item->isSelected() && dynamic_cast<ISelectable *>(item) != 0
                    && item->collidesWithItem(selected_item, Qt::ContainsItemBoundingRect)
                    && isInFrontOf(item, selected_item)) {
                QMT_CHECK(!_selected_items.contains(item));
                new_secondary_selected_items.insert(item);
            }
        }
    }

    // select all relations where both ends are primary or secondary selected
    foreach (DElement *element, _diagram->getDiagramElements()) {
        DRelation *relation = dynamic_cast<DRelation *>(element);
        if (relation) {
            QGraphicsItem *relation_item = _element_to_item_map.value(relation);
            QMT_CHECK(relation_item);
            DObject *end_a_object = _diagram_controller->findElement<DObject>(relation->getEndA(), _diagram);
            QMT_CHECK(end_a_object);
            QGraphicsItem *end_a_item = _element_to_item_map.value(end_a_object);
            QMT_CHECK(end_a_item);
            DObject *end_b_object = _diagram_controller->findElement<DObject>(relation->getEndB(), _diagram);
            QMT_CHECK(end_b_object);
            QGraphicsItem *end_b_item = _element_to_item_map.value(end_b_object);
            QMT_CHECK(end_b_item);
            if (!relation_item->isSelected()
                    && (_selected_items.contains(end_a_item) || new_secondary_selected_items.contains(end_a_item))
                    && (_selected_items.contains(end_b_item) || new_secondary_selected_items.contains(end_b_item))) {
                QMT_CHECK(!_selected_items.contains(relation_item));
                new_secondary_selected_items.insert(relation_item);
            }
        }
    }

    foreach (QGraphicsItem *item, _secondary_selected_items) {
        if (!new_secondary_selected_items.contains(item)) {
            ISelectable *selectable = dynamic_cast<ISelectable *>(item);
            QMT_CHECK(selectable);
            selectable->setSecondarySelected(false);
            selection_changed = true;
        }
    }
    foreach (QGraphicsItem *item, new_secondary_selected_items) {
        if (!_secondary_selected_items.contains(item)) {
            ISelectable *selectable = dynamic_cast<ISelectable *>(item);
            QMT_CHECK(selectable);
            selectable->setSecondarySelected(true);
            selection_changed = true;
        }
    }
    _secondary_selected_items = new_secondary_selected_items;

    QMT_CHECK((_selected_items & _secondary_selected_items).isEmpty());

    if (selection_changed) {
        _diagram_controller->breakUndoChain();
        emit selectionChanged(_diagram);
    }
}

void DiagramSceneModel::clearGraphicsScene()
{
    // save extra items from being deleted
    removeExtraSceneItems();
    _graphics_scene->clear();
    addExtraSceneItems();
    _graphics_items.clear();
    _item_to_element_map.clear();
    _element_to_item_map.clear();
    _selected_items.clear();
    _secondary_selected_items.clear();
    _focus_item = 0;
}

void DiagramSceneModel::removeExtraSceneItems()
{
    _latch_controller->removeFromGraphicsScene(_graphics_scene);
    _graphics_scene->removeItem(_origin_item);
}

void DiagramSceneModel::addExtraSceneItems()
{
    _graphics_scene->addItem(_origin_item);
    _latch_controller->addToGraphicsScene(_graphics_scene);
}

QGraphicsItem *DiagramSceneModel::createGraphicsItem(DElement *element)
{
    QMT_CHECK(element);
    QMT_CHECK(!_element_to_item_map.contains(element));

    CreationVisitor visitor(this);
    element->accept(&visitor);
    QGraphicsItem *item = visitor.getCreatedGraphicsItem();
    _item_to_element_map.insert(item, element);
    _element_to_item_map.insert(element, item);
    _graphics_scene->addItem(item);
    return item;
}

void DiagramSceneModel::updateGraphicsItem(QGraphicsItem *item, DElement *element)
{
    QMT_CHECK(item);
    QMT_CHECK(element);

    UpdateVisitor visitor(item, this);
    element->accept(&visitor);
}

void DiagramSceneModel::deleteGraphicsItem(QGraphicsItem *item, DElement *element)
{
    QMT_CHECK(_element_to_item_map.contains(element));
    QMT_CHECK(_item_to_element_map.contains(item));
    if (item == _focus_item) {
        unsetFocusItem();
    }
    _graphics_scene->removeItem(item);
    _element_to_item_map.remove(element);
    _item_to_element_map.remove(item);
    _selected_items.remove(item);
    _secondary_selected_items.remove(item);
    delete item;
}

void DiagramSceneModel::updateFocusItem(const QSet<QGraphicsItem *> &selected_items)
{
    QGraphicsItem *mouse_grabber_item = _graphics_scene->mouseGrabberItem();
    QGraphicsItem *focus_item = 0;
    ISelectable *selectable = 0;

    if (mouse_grabber_item && selected_items.contains(mouse_grabber_item)) {
        selectable = dynamic_cast<ISelectable *>(mouse_grabber_item);
        if (selectable) {
            focus_item = mouse_grabber_item;
        }
    }
    if (focus_item && focus_item != _focus_item) {
        unsetFocusItem();
        selectable->setFocusSelected(true);
        _focus_item = focus_item;
    } else if (_focus_item && !selected_items.contains(_focus_item)) {
        unsetFocusItem();
    }
}

void DiagramSceneModel::unsetFocusItem()
{
    if (_focus_item) {
        if (ISelectable *old_selectable = dynamic_cast<ISelectable *>(_focus_item)) {
            old_selectable->setFocusSelected(false);
        } else {
            QMT_CHECK(false);
        }
        _focus_item = 0;
    }
}

bool DiagramSceneModel::isInFrontOf(const QGraphicsItem *front_item, const QGraphicsItem *back_item)
{
    QMT_CHECK(front_item);
    QMT_CHECK(back_item);

    // shortcut for usual case of root items
    if (front_item->parentItem() == 0 && back_item->parentItem() == 0) {
        foreach (const QGraphicsItem *item, _graphics_scene->items()) {
            if (item == front_item) {
                return true;
            } else if (item == back_item) {
                return false;
            }
        }
        QMT_CHECK(false);
        return false;
    }

    // collect all anchestors of front item
    QList<const QGraphicsItem *> front_stack;
    const QGraphicsItem *iterator = front_item;
    while (iterator != 0) {
        front_stack.append(iterator);
        iterator = iterator->parentItem();
    }

    // collect all anchestors of back item
    QList<const QGraphicsItem *> back_stack;
    iterator = back_item;
    while (iterator != 0) {
        back_stack.append(iterator);
        iterator = iterator->parentItem();
    }

    // search lowest common anchestor
    int front_index = front_stack.size() - 1;
    int back_index = back_stack.size() - 1;
    while (front_index >= 0 && back_index >= 0 && front_stack.at(front_index) == back_stack.at(back_index)) {
        --front_index;
        --back_index;
    }

    if (front_index < 0 && back_index < 0) {
        QMT_CHECK(front_item == back_item);
        return false;
    } else if (front_index < 0) {
        // front item is higher in hierarchy and thus behind back item
        return false;
    } else if (back_index < 0) {
        // back item is higher in hierarchy and thus in behind front item
        return true;
    } else {
        front_item = front_stack.at(front_index);
        back_item = back_stack.at(back_index);
        QMT_CHECK(front_item != back_item);

        if (front_item->zValue() != back_item->zValue()) {
            return front_item->zValue() > back_item->zValue();
        } else {
            QList<QGraphicsItem *> children;
            if (front_index + 1 < front_stack.size()) {
                children = front_stack.at(front_index + 1)->childItems();
            } else {
                children = _graphics_scene->items(Qt::AscendingOrder);
            }
            foreach (const QGraphicsItem *item, children) {
                if (item == front_item) {
                    return false;
                } else if (item == back_item) {
                    return true;
                }
            }
            QMT_CHECK(false);
            return false;
        }
    }
}

}
