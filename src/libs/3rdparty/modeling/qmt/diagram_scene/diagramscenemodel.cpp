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
      m_diagramController(0),
      m_diagramSceneController(0),
      m_styleController(0),
      m_stereotypeController(0),
      m_diagram(0),
      m_graphicsScene(new DiagramGraphicsScene(this)),
      m_latchController(new LatchController(this)),
      m_busy(NOT_BUSY),
      m_originItem(new OriginItem()),
      m_focusItem(0)
{
    m_latchController->setDiagramSceneModel(this);
    connect(m_graphicsScene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));

    // add one item at origin to force scene rect to include origin always
    m_graphicsScene->addItem(m_originItem);

    m_latchController->addToGraphicsScene(m_graphicsScene);

}

DiagramSceneModel::~DiagramSceneModel()
{
    QMT_CHECK(m_busy == NOT_BUSY);
    m_latchController->removeFromGraphicsScene(m_graphicsScene);
    disconnect();
    if (m_diagramController) {
        disconnect(m_diagramController, 0, this, 0);
    }
    m_graphicsScene->deleteLater();
}

void DiagramSceneModel::setDiagramController(DiagramController *diagram_controller)
{
    if (m_diagramController == diagram_controller) {
        return;
    }
    if (m_diagramController) {
        disconnect(m_diagramController, 0, this, 0);
        m_diagramController = 0;
    }
    m_diagramController = diagram_controller;
    if (diagram_controller) {
        connect(m_diagramController, SIGNAL(beginResetAllDiagrams()), this, SLOT(onBeginResetAllDiagrams()));
        connect(m_diagramController, SIGNAL(endResetAllDiagrams()), this, SLOT(onEndResetAllDiagrams()));
        connect(m_diagramController, SIGNAL(beginResetDiagram(const MDiagram*)), this, SLOT(onBeginResetDiagram(const MDiagram*)));
        connect(m_diagramController, SIGNAL(endResetDiagram(const MDiagram*)), this, SLOT(onEndResetDiagram(const MDiagram*)));
        connect(m_diagramController, SIGNAL(beginUpdateElement(int,const MDiagram*)), this, SLOT(onBeginUpdateElement(int,const MDiagram*)));
        connect(m_diagramController, SIGNAL(endUpdateElement(int,const MDiagram*)), this, SLOT(onEndUpdateElement(int,const MDiagram*)));
        connect(m_diagramController, SIGNAL(beginInsertElement(int,const MDiagram*)), this, SLOT(onBeginInsertElement(int,const MDiagram*)));
        connect(m_diagramController, SIGNAL(endInsertElement(int,const MDiagram*)), this, SLOT(onEndInsertElement(int,const MDiagram*)));
        connect(m_diagramController, SIGNAL(beginRemoveElement(int,const MDiagram*)), this, SLOT(onBeginRemoveElement(int,const MDiagram*)));
        connect(m_diagramController, SIGNAL(endRemoveElement(int,const MDiagram*)), this, SLOT(onEndRemoveElement(int,const MDiagram*)));
    }
}

void DiagramSceneModel::setDiagramSceneController(DiagramSceneController *diagram_scene_controller)
{
    m_diagramSceneController = diagram_scene_controller;
}

void DiagramSceneModel::setStyleController(StyleController *style_controller)
{
    m_styleController = style_controller;
}

void DiagramSceneModel::setStereotypeController(StereotypeController *stereotype_controller)
{
    m_stereotypeController = stereotype_controller;
}

void DiagramSceneModel::setDiagram(MDiagram *diagram)
{
    if (m_diagram != diagram) {
        onBeginResetDiagram(diagram);
        m_diagram = diagram;
        onEndResetDiagram(diagram);
    }
}

QGraphicsScene *DiagramSceneModel::getGraphicsScene() const
{
    return m_graphicsScene;
}

bool DiagramSceneModel::hasSelection() const
{
    return !m_graphicsScene->selectedItems().isEmpty();
}

bool DiagramSceneModel::hasMultiObjectsSelection() const
{
    int count = 0;
    foreach (QGraphicsItem *item, m_graphicsScene->selectedItems()) {
        DElement *element = m_itemToElementMap.value(item);
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
    foreach (QGraphicsItem *item, m_graphicsScene->selectedItems()) {
        DElement *element = m_itemToElementMap.value(item);
        QMT_CHECK(element);
        selection.append(element->getUid(), m_diagram->getUid());
    }
    return selection;
}

DElement *DiagramSceneModel::findTopmostElement(const QPointF &scene_pos) const
{
    // fetch affected items from scene in correct drawing order to find topmost element
    QList<QGraphicsItem *> items = m_graphicsScene->items(scene_pos);
    foreach (QGraphicsItem *item, items) {
        if (m_graphicsItems.contains(item)) {
            return m_itemToElementMap.value(item);
        }
    }
    return 0;
}

QGraphicsItem *DiagramSceneModel::getGraphicsItem(DElement *element) const
{
    return m_elementToItemMap.value(element);
}

QGraphicsItem *DiagramSceneModel::getGraphicsItem(const Uid &uid) const
{
    return m_elementToItemMap.value(m_diagramController->findElement(uid, m_diagram));
}

bool DiagramSceneModel::isSelectedItem(QGraphicsItem *item) const
{
    return m_selectedItems.contains(item);
}

DElement *DiagramSceneModel::getElement(QGraphicsItem *item) const
{
    return m_itemToElementMap.value(item);
}

bool DiagramSceneModel::isElementEditable(const DElement *element) const
{
    IEditable *editable = dynamic_cast<IEditable *>(m_elementToItemMap.value(element));
    return editable != 0 && editable->isEditable();
}

void DiagramSceneModel::selectAllElements()
{
    foreach (QGraphicsItem *item, m_graphicsItems) {
        item->setSelected(true);
    }
}

void DiagramSceneModel::selectElement(DElement *element)
{
    QGraphicsItem *select_item = m_elementToItemMap.value(element);
    foreach (QGraphicsItem *item, m_selectedItems) {
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
    IEditable *editable = dynamic_cast<IEditable *>(m_elementToItemMap.value(element));
    if (editable != 0 && editable->isEditable()) {
        editable->edit();
    }
}

void DiagramSceneModel::copyToClipboard()
{
    QMimeData *mime_data = new QMimeData();

    // Selections would also render to the clipboard
    m_graphicsScene->clearSelection();
    removeExtraSceneItems();

    QRectF scene_bounding_rect = m_graphicsScene->itemsBoundingRect();

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
        m_graphicsScene->render(&painter,
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
        m_graphicsScene->render(&pdf_painter,
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
        m_graphicsScene->render(&svg_painter,
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

    QRectF scene_bounding_rect = m_graphicsScene->itemsBoundingRect();

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
    m_graphicsScene->render(&painter,
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

    QRectF scene_bounding_rect = m_graphicsScene->itemsBoundingRect();

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
    m_graphicsScene->render(&pdf_painter,
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
            foreach (QGraphicsItem *selected_item, m_selectedItems) {
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
        foreach (QGraphicsItem *item, m_selectedItems) {
            if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
                moveable->moveDelta(delta);
            }
        }
        foreach (QGraphicsItem *item, m_secondarySelectedItems) {
            if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
                moveable->moveDelta(delta);
            }
        }
    }
}

void DiagramSceneModel::alignSelectedItemsPositionOnRaster()
{
    foreach (QGraphicsItem *item, m_selectedItems) {
        if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
            moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
        }
    }
    foreach (QGraphicsItem *item, m_secondarySelectedItems) {
        if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
            moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
        }
    }
}

void DiagramSceneModel::onDoubleClickedItem(QGraphicsItem *item)
{
    DElement *element = m_itemToElementMap.value(item);
    if (item) {
        m_diagramSceneController->getElementTasks()->openElement(element, m_diagram);
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
        foreach (QGraphicsItem *candidate, m_graphicsItems) {
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
        foreach (QGraphicsItem *candidate, m_graphicsItems) {
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
        foreach (QGraphicsItem *candidate, m_graphicsItems) {
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
    emit diagramSceneActivated(m_diagram);
}

void DiagramSceneModel::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    updateFocusItem(QSet<QGraphicsItem *>::fromList(m_graphicsScene->selectedItems()));
    m_latchController->mousePressEventLatching(event);
    mousePressEventReparenting(event);
}

void DiagramSceneModel::mousePressEventReparenting(QGraphicsSceneMouseEvent *event)
{
    // TODO add keyboard event handler to change cursor also on modifier change without move
    mouseMoveEventReparenting(event);
}

void DiagramSceneModel::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    m_latchController->mouseMoveEventLatching(event);
    mouseMoveEventReparenting(event);
}

void DiagramSceneModel::mouseMoveEventReparenting(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::AltModifier) {
        // TODO show move cursor only if elements can be moved to underlaying element
        foreach (QGraphicsView *view, m_graphicsScene->views()) {
            // TODO find a better cursor that signals "move to this package"
            view->setCursor(QCursor(Qt::OpenHandCursor));
        }
    } else {
        foreach (QGraphicsView *view, m_graphicsScene->views()) {
            view->unsetCursor();
        }
    }
}

void DiagramSceneModel::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_latchController->mouseReleaseEventLatching(event);
    mouseReleaseEventReparenting(event);
}

void DiagramSceneModel::mouseReleaseEventReparenting(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::AltModifier) {
        ModelController *model_controller = getDiagramController()->getModelController();
        MPackage *new_owner = 0;
        QSet<QGraphicsItem *> selected_item_set = m_graphicsScene->selectedItems().toSet();
        QList<QGraphicsItem *> items_under_mouse = m_graphicsScene->items(event->scenePos());
        foreach (QGraphicsItem *item, items_under_mouse) {
            if (!selected_item_set.contains(item)) {
                // the item may be any graphics item not matching to a DElement
                DElement *element = m_itemToElementMap.value(item);
                if (element && element->getModelUid().isValid()) {
                    new_owner = model_controller->findElement<MPackage>(element->getModelUid());
                }
            }
            if (new_owner) {
                break;
            }
        }
        if (new_owner) {
            foreach (QGraphicsItem *item, m_graphicsScene->selectedItems()) {
                DElement *element = m_itemToElementMap.value(item);
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
    foreach (QGraphicsView *view, m_graphicsScene->views()) {
        view->unsetCursor();
    }
}

void DiagramSceneModel::onBeginResetAllDiagrams()
{
    onBeginResetDiagram(m_diagram);
}

void DiagramSceneModel::onEndResetAllDiagrams()
{
    onEndResetDiagram(m_diagram);
}

void DiagramSceneModel::onBeginResetDiagram(const MDiagram *diagram)
{
    QMT_CHECK(m_busy == NOT_BUSY);
    m_busy = RESET_DIAGRAM;
    if (diagram == m_diagram) {
        clearGraphicsScene();
    }
}

void DiagramSceneModel::onEndResetDiagram(const MDiagram *diagram)
{
    QMT_CHECK(m_busy == RESET_DIAGRAM);
    if (diagram == m_diagram) {
        QMT_CHECK(m_graphicsItems.size() == 0);
        // create all items and update graphics item from element initially
        foreach (DElement *element, diagram->getDiagramElements()) {
            QGraphicsItem *item = createGraphicsItem(element);
            m_graphicsItems.append(item);
            updateGraphicsItem(item, element);
        }
        // invalidate scene
        m_graphicsScene->invalidate();
        // update graphics items again so every item gets a correct list of colliding items
        foreach (DElement *element, diagram->getDiagramElements()) {
            updateGraphicsItem(m_elementToItemMap.value(element), element);
        }
    }
    m_busy = NOT_BUSY;
}

void DiagramSceneModel::onBeginUpdateElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(m_busy == NOT_BUSY);
    m_busy = UPDATE_ELEMENT;

}

void DiagramSceneModel::onEndUpdateElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(m_busy == UPDATE_ELEMENT);
    if (diagram == m_diagram) {
        QGraphicsItem *item = m_graphicsItems.at(row);
        updateGraphicsItem(item, diagram->getDiagramElements().at(row));
    }
    m_busy = NOT_BUSY;
}

void DiagramSceneModel::onBeginInsertElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(m_busy == NOT_BUSY);
    m_busy = INSERT_ELEMENT;
}

void DiagramSceneModel::onEndInsertElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(m_busy == INSERT_ELEMENT);
    QGraphicsItem *item = 0;
    if (diagram == m_diagram) {
        DElement *element = diagram->getDiagramElements().at(row);
        item = createGraphicsItem(element);
        m_graphicsItems.insert(row, item);
        updateGraphicsItem(item, element);
        m_graphicsScene->invalidate();
        updateGraphicsItem(item, element);
    }
    m_busy = NOT_BUSY;
}

void DiagramSceneModel::onBeginRemoveElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(m_busy == NOT_BUSY);
    if (diagram == m_diagram) {
        QGraphicsItem *item = m_graphicsItems.takeAt(row);
        deleteGraphicsItem(item, diagram->getDiagramElements().at(row));
    }
    m_busy = REMOVE_ELEMENT;
}

void DiagramSceneModel::onEndRemoveElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(m_busy == REMOVE_ELEMENT);
    m_busy = NOT_BUSY;
}

void DiagramSceneModel::onSelectionChanged()
{
    bool selection_changed = false;

    // collect and update all primary selected items (selected by user)
    QSet<QGraphicsItem *> new_selected_items = QSet<QGraphicsItem *>::fromList(m_graphicsScene->selectedItems());
    updateFocusItem(new_selected_items);
    foreach (QGraphicsItem *item, m_selectedItems) {
        if (!new_selected_items.contains(item)) {
            DElement *element = m_itemToElementMap.value(item);
            updateGraphicsItem(item, element);
            selection_changed = true;
        }
    }
    foreach (QGraphicsItem *item, new_selected_items) {
        if (!m_selectedItems.contains(item)) {
            DElement *element = m_itemToElementMap.value(item);
            updateGraphicsItem(item, element);
            selection_changed = true;
        }
    }
    m_selectedItems = new_selected_items;

    // collect and update all secondary selected items
    QSet<QGraphicsItem *> new_secondary_selected_items;

    // select all contained objects secondarily
    foreach (QGraphicsItem *selected_item, m_selectedItems) {
        foreach (QGraphicsItem *item, collectCollidingObjectItems(selected_item, COLLIDING_INNER_ITEMS)) {
            if (!item->isSelected() && dynamic_cast<ISelectable *>(item) != 0
                    && item->collidesWithItem(selected_item, Qt::ContainsItemBoundingRect)
                    && isInFrontOf(item, selected_item)) {
                QMT_CHECK(!m_selectedItems.contains(item));
                new_secondary_selected_items.insert(item);
            }
        }
    }

    // select all relations where both ends are primary or secondary selected
    foreach (DElement *element, m_diagram->getDiagramElements()) {
        DRelation *relation = dynamic_cast<DRelation *>(element);
        if (relation) {
            QGraphicsItem *relation_item = m_elementToItemMap.value(relation);
            QMT_CHECK(relation_item);
            DObject *end_a_object = m_diagramController->findElement<DObject>(relation->getEndA(), m_diagram);
            QMT_CHECK(end_a_object);
            QGraphicsItem *end_a_item = m_elementToItemMap.value(end_a_object);
            QMT_CHECK(end_a_item);
            DObject *end_b_object = m_diagramController->findElement<DObject>(relation->getEndB(), m_diagram);
            QMT_CHECK(end_b_object);
            QGraphicsItem *end_b_item = m_elementToItemMap.value(end_b_object);
            QMT_CHECK(end_b_item);
            if (!relation_item->isSelected()
                    && (m_selectedItems.contains(end_a_item) || new_secondary_selected_items.contains(end_a_item))
                    && (m_selectedItems.contains(end_b_item) || new_secondary_selected_items.contains(end_b_item))) {
                QMT_CHECK(!m_selectedItems.contains(relation_item));
                new_secondary_selected_items.insert(relation_item);
            }
        }
    }

    foreach (QGraphicsItem *item, m_secondarySelectedItems) {
        if (!new_secondary_selected_items.contains(item)) {
            ISelectable *selectable = dynamic_cast<ISelectable *>(item);
            QMT_CHECK(selectable);
            selectable->setSecondarySelected(false);
            selection_changed = true;
        }
    }
    foreach (QGraphicsItem *item, new_secondary_selected_items) {
        if (!m_secondarySelectedItems.contains(item)) {
            ISelectable *selectable = dynamic_cast<ISelectable *>(item);
            QMT_CHECK(selectable);
            selectable->setSecondarySelected(true);
            selection_changed = true;
        }
    }
    m_secondarySelectedItems = new_secondary_selected_items;

    QMT_CHECK((m_selectedItems & m_secondarySelectedItems).isEmpty());

    if (selection_changed) {
        m_diagramController->breakUndoChain();
        emit selectionChanged(m_diagram);
    }
}

void DiagramSceneModel::clearGraphicsScene()
{
    // save extra items from being deleted
    removeExtraSceneItems();
    m_graphicsScene->clear();
    addExtraSceneItems();
    m_graphicsItems.clear();
    m_itemToElementMap.clear();
    m_elementToItemMap.clear();
    m_selectedItems.clear();
    m_secondarySelectedItems.clear();
    m_focusItem = 0;
}

void DiagramSceneModel::removeExtraSceneItems()
{
    m_latchController->removeFromGraphicsScene(m_graphicsScene);
    m_graphicsScene->removeItem(m_originItem);
}

void DiagramSceneModel::addExtraSceneItems()
{
    m_graphicsScene->addItem(m_originItem);
    m_latchController->addToGraphicsScene(m_graphicsScene);
}

QGraphicsItem *DiagramSceneModel::createGraphicsItem(DElement *element)
{
    QMT_CHECK(element);
    QMT_CHECK(!m_elementToItemMap.contains(element));

    CreationVisitor visitor(this);
    element->accept(&visitor);
    QGraphicsItem *item = visitor.getCreatedGraphicsItem();
    m_itemToElementMap.insert(item, element);
    m_elementToItemMap.insert(element, item);
    m_graphicsScene->addItem(item);
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
    QMT_CHECK(m_elementToItemMap.contains(element));
    QMT_CHECK(m_itemToElementMap.contains(item));
    if (item == m_focusItem) {
        unsetFocusItem();
    }
    m_graphicsScene->removeItem(item);
    m_elementToItemMap.remove(element);
    m_itemToElementMap.remove(item);
    m_selectedItems.remove(item);
    m_secondarySelectedItems.remove(item);
    delete item;
}

void DiagramSceneModel::updateFocusItem(const QSet<QGraphicsItem *> &selected_items)
{
    QGraphicsItem *mouse_grabber_item = m_graphicsScene->mouseGrabberItem();
    QGraphicsItem *focus_item = 0;
    ISelectable *selectable = 0;

    if (mouse_grabber_item && selected_items.contains(mouse_grabber_item)) {
        selectable = dynamic_cast<ISelectable *>(mouse_grabber_item);
        if (selectable) {
            focus_item = mouse_grabber_item;
        }
    }
    if (focus_item && focus_item != m_focusItem) {
        unsetFocusItem();
        selectable->setFocusSelected(true);
        m_focusItem = focus_item;
    } else if (m_focusItem && !selected_items.contains(m_focusItem)) {
        unsetFocusItem();
    }
}

void DiagramSceneModel::unsetFocusItem()
{
    if (m_focusItem) {
        if (ISelectable *old_selectable = dynamic_cast<ISelectable *>(m_focusItem)) {
            old_selectable->setFocusSelected(false);
        } else {
            QMT_CHECK(false);
        }
        m_focusItem = 0;
    }
}

bool DiagramSceneModel::isInFrontOf(const QGraphicsItem *front_item, const QGraphicsItem *back_item)
{
    QMT_CHECK(front_item);
    QMT_CHECK(back_item);

    // shortcut for usual case of root items
    if (front_item->parentItem() == 0 && back_item->parentItem() == 0) {
        foreach (const QGraphicsItem *item, m_graphicsScene->items()) {
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
                children = m_graphicsScene->items(Qt::AscendingOrder);
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
