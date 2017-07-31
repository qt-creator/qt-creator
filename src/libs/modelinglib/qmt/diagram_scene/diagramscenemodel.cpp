/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
#include "qmt/diagram_scene/items/objectitem.h"
#include "qmt/diagram_scene/items/swimlaneitem.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/tasks/diagramscenecontroller.h"
#include "qmt/tasks/ielementtasks.h"

#include "utils/asconst.h"

#include <QSet>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <QBuffer>
#include <QPdfWriter>
#include <QFile>

#ifndef QT_NO_SVG
#include <QtSvg/QSvgGenerator>
#endif

namespace qmt {

class DiagramSceneModel::OriginItem : public QGraphicsItem
{
public:
    explicit OriginItem(QGraphicsItem *parent = nullptr)
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
      m_graphicsScene(new DiagramGraphicsScene(this)),
      m_latchController(new LatchController(this)),
      m_originItem(new OriginItem())
{
    m_latchController->setDiagramSceneModel(this);
    connect(m_graphicsScene, &QGraphicsScene::selectionChanged,
            this, &DiagramSceneModel::onSelectionChanged);

    // add one item at origin to force scene rect to include origin always
    m_graphicsScene->addItem(m_originItem);

    m_latchController->addToGraphicsScene(m_graphicsScene);
}

DiagramSceneModel::~DiagramSceneModel()
{
    QMT_CHECK(m_busyState == NotBusy);
    m_latchController->removeFromGraphicsScene(m_graphicsScene);
    disconnect();
    if (m_diagramController)
        disconnect(m_diagramController, nullptr, this, nullptr);
    m_graphicsScene->deleteLater();
}

void DiagramSceneModel::setDiagramController(DiagramController *diagramController)
{
    if (m_diagramController == diagramController)
        return;
    if (m_diagramController) {
        disconnect(m_diagramController, nullptr, this, nullptr);
        m_diagramController = nullptr;
    }
    m_diagramController = diagramController;
    if (diagramController) {
        connect(m_diagramController, &DiagramController::beginResetAllDiagrams,
                this, &DiagramSceneModel::onBeginResetAllDiagrams);
        connect(m_diagramController, &DiagramController::endResetAllDiagrams,
                this, &DiagramSceneModel::onEndResetAllDiagrams);
        connect(m_diagramController, &DiagramController::beginResetDiagram,
                this, &DiagramSceneModel::onBeginResetDiagram);
        connect(m_diagramController, &DiagramController::endResetDiagram,
                this, &DiagramSceneModel::onEndResetDiagram);
        connect(m_diagramController, &DiagramController::beginUpdateElement,
                this, &DiagramSceneModel::onBeginUpdateElement);
        connect(m_diagramController, &DiagramController::endUpdateElement,
                this, &DiagramSceneModel::onEndUpdateElement);
        connect(m_diagramController, &DiagramController::beginInsertElement,
                this, &DiagramSceneModel::onBeginInsertElement);
        connect(m_diagramController, &DiagramController::endInsertElement,
                this, &DiagramSceneModel::onEndInsertElement);
        connect(m_diagramController, &DiagramController::beginRemoveElement,
                this, &DiagramSceneModel::onBeginRemoveElement);
        connect(m_diagramController, &DiagramController::endRemoveElement,
                this, &DiagramSceneModel::onEndRemoveElement);
    }
}

void DiagramSceneModel::setDiagramSceneController(DiagramSceneController *diagramSceneController)
{
    m_diagramSceneController = diagramSceneController;
}

void DiagramSceneModel::setStyleController(StyleController *styleController)
{
    m_styleController = styleController;
}

void DiagramSceneModel::setStereotypeController(StereotypeController *stereotypeController)
{
    m_stereotypeController = stereotypeController;
}

void DiagramSceneModel::setDiagram(MDiagram *diagram)
{
    if (m_diagram != diagram) {
        onBeginResetDiagram(diagram);
        m_diagram = diagram;
        onEndResetDiagram(diagram);
    }
}

QGraphicsScene *DiagramSceneModel::graphicsScene() const
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
        if (dynamic_cast<DObject *>(element)) {
            ++count;
            if (count > 1)
                return true;
        }
    }
    return false;
}

DSelection DiagramSceneModel::selectedElements() const
{
    DSelection selection;
    foreach (QGraphicsItem *item, m_graphicsScene->selectedItems()) {
        DElement *element = m_itemToElementMap.value(item);
        QMT_ASSERT(element, return selection);
        selection.append(element->uid(), m_diagram->uid());
    }
    return selection;
}

DElement *DiagramSceneModel::findTopmostElement(const QPointF &scenePos) const
{
    // fetch affected items from scene in correct drawing order to find topmost element
    QList<QGraphicsItem *> items = m_graphicsScene->items(scenePos);
    foreach (QGraphicsItem *item, items) {
        if (m_graphicsItems.contains(item))
            return m_itemToElementMap.value(item);
    }
    return nullptr;
}

DObject *DiagramSceneModel::findTopmostObject(const QPointF &scenePos) const
{
    ObjectItem *item = findTopmostObjectItem(scenePos);
    if (!item)
        return nullptr;
    return item->object();
}

ObjectItem *DiagramSceneModel::findTopmostObjectItem(const QPointF &scenePos) const
{
    // fetch affected items from scene in correct drawing order to find topmost element
    const QList<QGraphicsItem *> items = m_graphicsScene->items(scenePos);
    for (QGraphicsItem *item : Utils::asConst(items)) {
        if (m_graphicsItems.contains(item)) {
            DObject *object = dynamic_cast<DObject *>(m_itemToElementMap.value(item));
            if (object)
                return dynamic_cast<ObjectItem *>(item);
        }
    }
    return nullptr;
}

QGraphicsItem *DiagramSceneModel::graphicsItem(DElement *element) const
{
    return m_elementToItemMap.value(element);
}

QGraphicsItem *DiagramSceneModel::graphicsItem(const Uid &uid) const
{
    return m_elementToItemMap.value(m_diagramController->findElement(uid, m_diagram));
}

bool DiagramSceneModel::isSelectedItem(QGraphicsItem *item) const
{
    return m_selectedItems.contains(item);
}

DElement *DiagramSceneModel::element(QGraphicsItem *item) const
{
    return m_itemToElementMap.value(item);
}

bool DiagramSceneModel::isElementEditable(const DElement *element) const
{
    auto editable = dynamic_cast<IEditable *>(m_elementToItemMap.value(element));
    return editable && editable->isEditable();
}

void DiagramSceneModel::selectAllElements()
{
    foreach (QGraphicsItem *item, m_graphicsItems)
        item->setSelected(true);
}

void DiagramSceneModel::selectElement(DElement *element)
{
    QGraphicsItem *selectItem = m_elementToItemMap.value(element);
    foreach (QGraphicsItem *item, m_selectedItems) {
        if (item != selectItem)
            item->setSelected(false);
    }
    if (selectItem)
        selectItem->setSelected(true);
}

void DiagramSceneModel::editElement(DElement *element)
{
    auto editable = dynamic_cast<IEditable *>(m_elementToItemMap.value(element));
    if (editable && editable->isEditable())
        editable->edit();
}

void DiagramSceneModel::copyToClipboard()
{
    auto mimeData = new QMimeData();

    QSet<QGraphicsItem *> selectedItems = m_selectedItems;
    QSet<QGraphicsItem *> secondarySelectedItems = m_secondarySelectedItems;
    QGraphicsItem *focusItem = m_focusItem;
    // Selections would also render to the clipboard
    m_graphicsScene->clearSelection();
    removeExtraSceneItems();

    bool copyAll = selectedItems.isEmpty() && secondarySelectedItems.isEmpty();
    QRectF sceneBoundingRect;
    if (copyAll) {
        sceneBoundingRect = m_graphicsScene->itemsBoundingRect();
    } else {
        foreach (QGraphicsItem *item, m_graphicsItems) {
            if (selectedItems.contains(item) || secondarySelectedItems.contains(item))
                sceneBoundingRect |= item->mapRectToScene(item->boundingRect());
            else
                item->hide();
        }
    }

    {
        // Create the image with the size of the shrunk scene
        const int scaleFactor = 4;
        const int border = 4;
        const int baseDpi = 75;
        const int dotsPerMeter = 10000 * baseDpi / 254;
        QSize imageSize = sceneBoundingRect.size().toSize();
        imageSize += QSize(2 * border, 2 * border);
        imageSize *= scaleFactor;
        QImage image(imageSize, QImage::Format_ARGB32);
        image.setDotsPerMeterX(dotsPerMeter * scaleFactor);
        image.setDotsPerMeterY(dotsPerMeter * scaleFactor);
        image.fill(Qt::white);
        QPainter painter;
        painter.begin(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        m_graphicsScene->render(&painter,
                                QRectF(border, border,
                                       painter.device()->width() - 2 * border,
                                       painter.device()->height() - 2 * border),
                                sceneBoundingRect);
        painter.end();
        mimeData->setImageData(image);
    }

    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);

    if (!copyAll) {
        // TODO once an annotation item had focus the call to show() will give it focus again. Bug in Qt?
        foreach (QGraphicsItem *item, m_graphicsItems)
            item->show();
    }

    addExtraSceneItems();

    foreach (QGraphicsItem *item, selectedItems)
        item->setSelected(true);

    // reset focus item
    if (focusItem) {
        ISelectable *selectable = dynamic_cast<ISelectable *>(focusItem);
        if (selectable) {
            selectable->setFocusSelected(true);
            m_focusItem = focusItem;
        }
    }
}

bool DiagramSceneModel::exportImage(const QString &fileName)
{
    // TODO support exporting selected elements only
    removeExtraSceneItems();

    QRectF sceneBoundingRect = m_graphicsScene->itemsBoundingRect();

    // Create the image with the size of the shrunk scene
    const int scaleFactor = 1;
    const int border = 5;
    const int baseDpi = 75;
    const int dotsPerMeter = 10000 * baseDpi / 254;

    QSize imageSize = sceneBoundingRect.size().toSize();
    imageSize += QSize(2 * border, 2 * border);
    imageSize *= scaleFactor;

    QImage image(imageSize, QImage::Format_ARGB32);
    image.setDotsPerMeterX(dotsPerMeter * scaleFactor);
    image.setDotsPerMeterY(dotsPerMeter * scaleFactor);
    image.fill(Qt::white);

    QPainter painter;
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_graphicsScene->render(&painter,
                            QRectF(border, border,
                                   painter.device()->width() - 2 * border,
                                   painter.device()->height() - 2 * border),
                            sceneBoundingRect);
    painter.end();

    bool success = image.save(fileName);
    addExtraSceneItems();
    return success;
}

bool DiagramSceneModel::exportPdf(const QString &fileName)
{
    // TODO support exporting selected elements only
    removeExtraSceneItems();

    QRectF sceneBoundingRect = m_graphicsScene->itemsBoundingRect();

    const double scaleFactor = 1.0;
    const double border = 5;
    const double baseDpi = 100;
    const double dotsPerMm = 25.4 / baseDpi;

    QSizeF pageSize = sceneBoundingRect.size();
    pageSize += QSizeF(2.0 * border, 2.0 * border);
    pageSize *= scaleFactor;

    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPdfWriter::Custom);
    pdfWriter.setPageSizeMM(pageSize * dotsPerMm);

    QPainter pdfPainter;
    pdfPainter.begin(&pdfWriter);
    m_graphicsScene->render(&pdfPainter,
                            QRectF(border, border,
                                   pdfPainter.device()->width() - 2 * border,
                                   pdfPainter.device()->height() - 2 * border),
                            sceneBoundingRect);
    pdfPainter.end();

    addExtraSceneItems();

    // TODO how to know that file was successfully created?
    return true;
}

bool DiagramSceneModel::exportSvg(const QString &fileName)
{
#ifndef QT_NO_SVG
    // TODO support exporting selected elements only
    removeExtraSceneItems();

    QRectF sceneBoundingRect = m_graphicsScene->itemsBoundingRect();

    const double border = 5;

    QSvgGenerator svgGenerator;
    svgGenerator.setFileName(fileName);
    QSize svgSceneSize = sceneBoundingRect.size().toSize();
    svgGenerator.setSize(svgSceneSize);
    svgGenerator.setViewBox(QRect(QPoint(0,0), svgSceneSize));
    QPainter svgPainter;
    svgPainter.begin(&svgGenerator);
    svgPainter.setRenderHint(QPainter::Antialiasing);
    m_graphicsScene->render(&svgPainter,
                            QRectF(border, border,
                                   svgPainter.device()->width() - 2 * border,
                                   svgPainter.device()->height() - 2 * border),
                            sceneBoundingRect);
    svgPainter.end();

    addExtraSceneItems();

    // TODO how to know that file was successfully created?
    return true;
#else // QT_NO_SVG
    Q_UNUSED(fileName);
    return false;
#endif // QT_NO_SVG
}

void DiagramSceneModel::selectItem(QGraphicsItem *item, bool multiSelect)
{
    if (!multiSelect) {
        if (!item->isSelected()) {
            foreach (QGraphicsItem *selectedItem, m_selectedItems) {
                if (selectedItem != item)
                    selectedItem->setSelected(false);
            }
            item->setSelected(true);
        }
    } else {
        item->setSelected(!item->isSelected());
    }
}

void DiagramSceneModel::moveSelectedItems(QGraphicsItem *grabbedItem, const QPointF &delta)
{
    Q_UNUSED(grabbedItem);

    if (delta != QPointF(0.0, 0.0)) {
        foreach (QGraphicsItem *item, m_selectedItems) {
            if (auto moveable = dynamic_cast<IMoveable *>(item))
                moveable->moveDelta(delta);
        }
        foreach (QGraphicsItem *item, m_secondarySelectedItems) {
            if (auto moveable = dynamic_cast<IMoveable *>(item))
                moveable->moveDelta(delta);
        }
    }
}

void DiagramSceneModel::alignSelectedItemsPositionOnRaster()
{
    foreach (QGraphicsItem *item, m_selectedItems) {
        if (auto moveable = dynamic_cast<IMoveable *>(item))
            moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
    }
    foreach (QGraphicsItem *item, m_secondarySelectedItems) {
        if (auto moveable = dynamic_cast<IMoveable *>(item))
            moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
    }
}

void DiagramSceneModel::onDoubleClickedItem(QGraphicsItem *item)
{
    DElement *element = m_itemToElementMap.value(item);
    if (item)
        m_diagramSceneController->elementTasks()->openElement(element, m_diagram);
}

QList<QGraphicsItem *> DiagramSceneModel::collectCollidingObjectItems(const QGraphicsItem *item,
                                                                      CollidingMode collidingMode) const
{
    QList<QGraphicsItem *> collidingItems;

    auto resizable = dynamic_cast<const IResizable *>(item);
    if (!resizable)
        return collidingItems;
    QRectF rect = resizable->rect();
    rect.translate(resizable->pos());

    switch (collidingMode) {
    case CollidingInnerItems:
        foreach (QGraphicsItem *candidate, m_graphicsItems) {
            if (auto candidateResizable = dynamic_cast<const IResizable *>(candidate)) {
                QRectF candidateRect = candidateResizable->rect();
                candidateRect.translate(candidateResizable->pos());
                if (candidateRect.left() >= rect.left() && candidateRect.right() <= rect.right()
                        && candidateRect.top() >= rect.top() && candidateRect.bottom() <= rect.bottom()) {
                    collidingItems.append(candidate);
                }
            }
        }
        break;
    case CollidingItems:
        foreach (QGraphicsItem *candidate, m_graphicsItems) {
            if (auto candidateResizable = dynamic_cast<const IResizable *>(candidate)) {
                QRectF candidateRect = candidateResizable->rect();
                candidateRect.translate(candidateResizable->pos());
                if (candidateRect.left() <= rect.right() && candidateRect.right() >= rect.left()
                        && candidateRect.top() <= rect.bottom() && candidateRect.bottom() >= rect.top()) {
                    collidingItems.append(candidate);
                }
            }
        }
        break;
    case CollidingOuterItems:
        foreach (QGraphicsItem *candidate, m_graphicsItems) {
            if (auto candidateResizable = dynamic_cast<const IResizable *>(candidate)) {
                QRectF candidateRect = candidateResizable->rect();
                candidateRect.translate(candidateResizable->pos());
                if (candidateRect.left() <= rect.left() && candidateRect.right() >= rect.right()
                        && candidateRect.top() <= rect.top() && candidateRect.bottom() >= rect.bottom()) {
                    collidingItems.append(candidate);
                }
            }
        }
        break;
    }
    return collidingItems;
}

void DiagramSceneModel::sceneActivated()
{
    emit diagramSceneActivated(m_diagram);
}

void DiagramSceneModel::keyPressEvent(QKeyEvent *event)
{
    m_latchController->keyPressEventLatching(event);
}

void DiagramSceneModel::keyReleaseEvent(QKeyEvent *event)
{
    m_latchController->keyReleaseEventLatching(event);
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
        foreach (QGraphicsView *view, m_graphicsScene->views())
            view->unsetCursor();
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
        ModelController *modelController = diagramController()->modelController();
        MPackage *newOwner = nullptr;
        QSet<QGraphicsItem *> selectedItemSet = m_graphicsScene->selectedItems().toSet();
        QList<QGraphicsItem *> itemsUnderMouse = m_graphicsScene->items(event->scenePos());
        foreach (QGraphicsItem *item, itemsUnderMouse) {
            if (!selectedItemSet.contains(item)) {
                // the item may be any graphics item not matching to a DElement
                DElement *element = m_itemToElementMap.value(item);
                if (element && element->modelUid().isValid())
                    newOwner = modelController->findElement<MPackage>(element->modelUid());
            }
            if (newOwner)
                break;
        }
        if (newOwner) {
            foreach (QGraphicsItem *item, m_graphicsScene->selectedItems()) {
                DElement *element = m_itemToElementMap.value(item);
                QMT_ASSERT(element, return);
                if (element->modelUid().isValid()) {
                    MObject *modelObject = modelController->findObject(element->modelUid());
                    if (modelObject) {
                        if (newOwner != modelObject->owner())
                            modelController->moveObject(newOwner, modelObject);
                    }
                }
            }
        }
    }
    foreach (QGraphicsView *view, m_graphicsScene->views())
        view->unsetCursor();
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
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = ResetDiagram;
    if (diagram == m_diagram)
        clearGraphicsScene();
}

void DiagramSceneModel::onEndResetDiagram(const MDiagram *diagram)
{
    QMT_CHECK(m_busyState == ResetDiagram);
    if (diagram == m_diagram) {
        QMT_CHECK(m_graphicsItems.size() == 0);
        // create all items and update graphics item from element initially
        foreach (DElement *element, diagram->diagramElements()) {
            QGraphicsItem *item = createGraphicsItem(element);
            m_graphicsItems.append(item);
            updateGraphicsItem(item, element);
        }
        // invalidate scene
        m_graphicsScene->invalidate();
        // update graphics items again so every item gets a correct list of colliding items
        foreach (DElement *element, diagram->diagramElements())
            updateGraphicsItem(m_elementToItemMap.value(element), element);
        recalcSceneRectSize();
    }
    m_busyState = NotBusy;
}

void DiagramSceneModel::onBeginUpdateElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = UpdateElement;

}

void DiagramSceneModel::onEndUpdateElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(m_busyState == UpdateElement);
    if (diagram == m_diagram) {
        QGraphicsItem *item = m_graphicsItems.at(row);
        updateGraphicsItem(item, diagram->diagramElements().at(row));
        recalcSceneRectSize();
    }
    m_busyState = NotBusy;
}

void DiagramSceneModel::onBeginInsertElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(m_busyState == NotBusy);
    m_busyState = InsertElement;
}

void DiagramSceneModel::onEndInsertElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(m_busyState == InsertElement);
    QGraphicsItem *item = nullptr;
    if (diagram == m_diagram) {
        DElement *element = diagram->diagramElements().at(row);
        item = createGraphicsItem(element);
        m_graphicsItems.insert(row, item);
        updateGraphicsItem(item, element);
        m_graphicsScene->invalidate();
        updateGraphicsItem(item, element);
        recalcSceneRectSize();
    }
    m_busyState = NotBusy;
}

void DiagramSceneModel::onBeginRemoveElement(int row, const MDiagram *diagram)
{
    QMT_CHECK(m_busyState == NotBusy);
    if (diagram == m_diagram) {
        QGraphicsItem *item = m_graphicsItems.takeAt(row);
        deleteGraphicsItem(item, diagram->diagramElements().at(row));
        recalcSceneRectSize();
    }
    m_busyState = RemoveElement;
}

void DiagramSceneModel::onEndRemoveElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
    QMT_CHECK(m_busyState == RemoveElement);
    m_busyState = NotBusy;
}

void DiagramSceneModel::onSelectionChanged()
{
    bool selectionChanged = false;

    // collect and update all primary selected items (selected by user)
    QSet<QGraphicsItem *> newSelectedItems = QSet<QGraphicsItem *>::fromList(m_graphicsScene->selectedItems());
    updateFocusItem(newSelectedItems);
    foreach (QGraphicsItem *item, m_selectedItems) {
        if (!newSelectedItems.contains(item)) {
            DElement *element = m_itemToElementMap.value(item);
            updateGraphicsItem(item, element);
            selectionChanged = true;
        }
    }
    foreach (QGraphicsItem *item, newSelectedItems) {
        if (!m_selectedItems.contains(item)) {
            DElement *element = m_itemToElementMap.value(item);
            updateGraphicsItem(item, element);
            selectionChanged = true;
        }
    }
    m_selectedItems = newSelectedItems;

    // collect and update all secondary selected items
    QSet<QGraphicsItem *> newSecondarySelectedItems;

    // select all contained objects secondarily
    foreach (QGraphicsItem *selectedItem, m_selectedItems) {
        foreach (QGraphicsItem *item, collectCollidingObjectItems(selectedItem, CollidingInnerItems)) {
            if (!item->isSelected() && dynamic_cast<ISelectable *>(item)
                    && item->collidesWithItem(selectedItem, Qt::ContainsItemBoundingRect)
                    && isInFrontOf(item, selectedItem)) {
                QMT_CHECK(!m_selectedItems.contains(item));
                newSecondarySelectedItems.insert(item);
            }
        }
    }

    // select more items secondarily
    for (QGraphicsItem *selectedItem : Utils::asConst(m_selectedItems)) {
        if (auto selectable = dynamic_cast<ISelectable *>(selectedItem)) {
            QRectF boundary = selectable->getSecondarySelectionBoundary();
            if (!boundary.isEmpty()) {
                for (QGraphicsItem *item : Utils::asConst(m_graphicsItems)) {
                    if (auto secondarySelectable = dynamic_cast<ISelectable *>(item)) {
                        if (!item->isSelected() && !secondarySelectable->isSecondarySelected()) {
                            secondarySelectable->setBoundarySelected(boundary, true);
                            QMT_CHECK(!m_selectedItems.contains(item));
                            QMT_CHECK(!m_secondarySelectedItems.contains(item));
                            if (secondarySelectable->isSecondarySelected())
                                newSecondarySelectedItems.insert(item);
                        }
                    }
                }
            }
        }
    }


    // select all relations where both ends are primary or secondary selected
    foreach (DElement *element, m_diagram->diagramElements()) {
        auto relation = dynamic_cast<DRelation *>(element);
        if (relation) {
            QGraphicsItem *relationItem = m_elementToItemMap.value(relation);
            QMT_ASSERT(relationItem, return);
            DObject *endAObject = m_diagramController->findElement<DObject>(relation->endAUid(), m_diagram);
            QMT_ASSERT(endAObject, return);
            QGraphicsItem *endAItem = m_elementToItemMap.value(endAObject);
            QMT_ASSERT(endAItem, return);
            DObject *endBObject = m_diagramController->findElement<DObject>(relation->endBUid(), m_diagram);
            QMT_ASSERT(endBObject, return);
            QGraphicsItem *endBItem = m_elementToItemMap.value(endBObject);
            QMT_ASSERT(endBItem, return);
            if (relationItem && !relationItem->isSelected()
                    && (m_selectedItems.contains(endAItem) || newSecondarySelectedItems.contains(endAItem))
                    && (m_selectedItems.contains(endBItem) || newSecondarySelectedItems.contains(endBItem))) {
                QMT_CHECK(!m_selectedItems.contains(relationItem));
                newSecondarySelectedItems.insert(relationItem);
            }
        }
    }

    foreach (QGraphicsItem *item, m_secondarySelectedItems) {
        if (!newSecondarySelectedItems.contains(item)) {
            auto selectable = dynamic_cast<ISelectable *>(item);
            QMT_ASSERT(selectable, return);
            selectable->setSecondarySelected(false);
            selectionChanged = true;
        }
    }
    foreach (QGraphicsItem *item, newSecondarySelectedItems) {
        if (!m_secondarySelectedItems.contains(item)) {
            auto selectable = dynamic_cast<ISelectable *>(item);
            QMT_ASSERT(selectable, return);
            selectable->setSecondarySelected(true);
            selectionChanged = true;
        }
    }
    m_secondarySelectedItems = newSecondarySelectedItems;

    QMT_CHECK((m_selectedItems & m_secondarySelectedItems).isEmpty());

    if (selectionChanged) {
        m_diagramController->breakUndoChain();
        emit selectionHasChanged(m_diagram);
    }
}

void DiagramSceneModel::clearGraphicsScene()
{
    m_graphicsItems.clear();
    m_itemToElementMap.clear();
    m_elementToItemMap.clear();
    m_selectedItems.clear();
    m_secondarySelectedItems.clear();
    m_focusItem = nullptr;
    // save extra items from being deleted
    removeExtraSceneItems();
    m_graphicsScene->clear();
    addExtraSceneItems();
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

void DiagramSceneModel::recalcSceneRectSize()
{
    QRectF sceneRect = m_originItem->mapRectToScene(m_originItem->boundingRect());
    for (QGraphicsItem *item : Utils::asConst(m_graphicsItems)) {
        // TODO use an interface to update sceneRect by item
        if (!dynamic_cast<SwimlaneItem *>(item))
            sceneRect |= item->mapRectToScene(item->boundingRect());
    }
    emit sceneRectChanged(sceneRect);
}

QGraphicsItem *DiagramSceneModel::createGraphicsItem(DElement *element)
{
    QMT_ASSERT(element, return nullptr);
    QMT_CHECK(!m_elementToItemMap.contains(element));

    CreationVisitor visitor(this);
    element->accept(&visitor);
    QGraphicsItem *item = visitor.createdGraphicsItem();
    m_itemToElementMap.insert(item, element);
    m_elementToItemMap.insert(element, item);
    m_graphicsScene->addItem(item);
    return item;
}

void DiagramSceneModel::updateGraphicsItem(QGraphicsItem *item, DElement *element)
{
    QMT_ASSERT(item, return);
    QMT_ASSERT(element, return);

    UpdateVisitor visitor(item, this);
    element->accept(&visitor);
}

void DiagramSceneModel::deleteGraphicsItem(QGraphicsItem *item, DElement *element)
{
    QMT_CHECK(m_elementToItemMap.contains(element));
    QMT_CHECK(m_itemToElementMap.contains(item));
    if (item == m_focusItem)
        unsetFocusItem();
    m_graphicsScene->removeItem(item);
    m_elementToItemMap.remove(element);
    m_itemToElementMap.remove(item);
    m_selectedItems.remove(item);
    m_secondarySelectedItems.remove(item);
    delete item;
}

void DiagramSceneModel::updateFocusItem(const QSet<QGraphicsItem *> &selectedItems)
{
    QGraphicsItem *mouseGrabberItem = m_graphicsScene->mouseGrabberItem();
    QGraphicsItem *focusItem = nullptr;
    ISelectable *selectable = nullptr;

    if (mouseGrabberItem && selectedItems.contains(mouseGrabberItem)) {
        selectable = dynamic_cast<ISelectable *>(mouseGrabberItem);
        if (selectable)
            focusItem = mouseGrabberItem;
    }
    if (focusItem && focusItem != m_focusItem) {
        unsetFocusItem();
        selectable->setFocusSelected(true);
        m_focusItem = focusItem;
    } else if (m_focusItem && !selectedItems.contains(m_focusItem)) {
        unsetFocusItem();
    }
}

void DiagramSceneModel::unsetFocusItem()
{
    if (m_focusItem) {
        if (auto oldSelectable = dynamic_cast<ISelectable *>(m_focusItem))
            oldSelectable->setFocusSelected(false);
        else
            QMT_CHECK(false);
        m_focusItem = nullptr;
    }
}

bool DiagramSceneModel::isInFrontOf(const QGraphicsItem *frontItem, const QGraphicsItem *backItem)
{
    QMT_ASSERT(frontItem, return false);
    QMT_ASSERT(backItem, return false);

    // shortcut for usual case of root items
    if (!frontItem->parentItem() && !backItem->parentItem()) {
        foreach (const QGraphicsItem *item, m_graphicsScene->items()) {
            if (item == frontItem)
                return true;
            else if (item == backItem)
                return false;
        }
        QMT_CHECK(false);
        return false;
    }

    // collect all anchestors of front item
    QList<const QGraphicsItem *> frontStack;
    const QGraphicsItem *iterator = frontItem;
    while (iterator) {
        frontStack.append(iterator);
        iterator = iterator->parentItem();
    }

    // collect all anchestors of back item
    QList<const QGraphicsItem *> backStack;
    iterator = backItem;
    while (iterator) {
        backStack.append(iterator);
        iterator = iterator->parentItem();
    }

    // search lowest common anchestor
    int frontIndex = frontStack.size() - 1;
    int backIndex = backStack.size() - 1;
    while (frontIndex >= 0 && backIndex >= 0 && frontStack.at(frontIndex) == backStack.at(backIndex)) {
        --frontIndex;
        --backIndex;
    }

    if (frontIndex < 0 && backIndex < 0) {
        QMT_CHECK(frontItem == backItem);
        return false;
    } else if (frontIndex < 0) {
        // front item is higher in hierarchy and thus behind back item
        return false;
    } else if (backIndex < 0) {
        // back item is higher in hierarchy and thus in behind front item
        return true;
    } else {
        frontItem = frontStack.at(frontIndex);
        backItem = backStack.at(backIndex);
        QMT_CHECK(frontItem != backItem);

        if (frontItem->zValue() != backItem->zValue()) {
            return frontItem->zValue() > backItem->zValue();
        } else {
            QList<QGraphicsItem *> children;
            if (frontIndex + 1 < frontStack.size())
                children = frontStack.at(frontIndex + 1)->childItems();
            else
                children = m_graphicsScene->items(Qt::AscendingOrder);
            foreach (const QGraphicsItem *item, children) {
                if (item == frontItem)
                    return false;
                else if (item == backItem)
                    return true;
            }
            QMT_CHECK(false);
            return false;
        }
    }
}

} // namespace qmt
