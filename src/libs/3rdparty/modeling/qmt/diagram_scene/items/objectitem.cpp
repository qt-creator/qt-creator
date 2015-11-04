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

ObjectItem::ObjectItem(DObject *object, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_object(object),
      m_diagramSceneModel(diagramSceneModel),
      m_secondarySelected(false),
      m_focusSelected(false),
      m_stereotypeIconDisplay(StereotypeIcon::DISPLAY_LABEL),
      m_stereotypes(0),
      m_stereotypeIcon(0),
      m_selectionMarker(0),
      m_horizontalAlignButtons(0),
      m_verticalAlignButtons(0)
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
    return m_object->getPos();
}

QRectF ObjectItem::getRect() const
{
    return m_object->getRect();
}

void ObjectItem::setPosAndRect(const QPointF &originalPos, const QRectF &originalRect, const QPointF &topLeftDelta, const QPointF &bottomRightDelta)
{
    QPointF newPos = originalPos;
    QRectF newRect = originalRect;
    GeometryUtilities::adjustPosAndRect(&newPos, &newRect, topLeftDelta, bottomRightDelta, QPointF(0.5, 0.5));
    if (newPos != m_object->getPos() || newRect != m_object->getRect()) {
        m_diagramSceneModel->getDiagramController()->startUpdateElement(m_object, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
        m_object->setPos(newPos);
        if (newRect.size() != m_object->getRect().size()) {
            m_object->setAutoSize(false);
        }
        m_object->setRect(newRect);
        m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_object, m_diagramSceneModel->getDiagram(), false);
    }
}

void ObjectItem::alignItemSizeToRaster(IResizable::Side adjustHorizontalSide, IResizable::Side adjustVerticalSide, double rasterWidth, double rasterHeight)
{
    QPointF pos = m_object->getPos();
    QRectF rect = m_object->getRect();

    double horizDelta = rect.width() - qRound(rect.width() / rasterWidth) * rasterWidth;
    double vertDelta = rect.height() - qRound(rect.height() / rasterHeight) * rasterHeight;

    // make sure the new size is at least the minimum size
    QSizeF minimumSize = getMinimumSize();
    while (rect.width() + horizDelta < minimumSize.width()) {
        horizDelta += rasterWidth;
    }
    while (rect.height() + vertDelta < minimumSize.height()) {
        vertDelta += rasterHeight;
    }

    double leftDelta = 0.0;
    double rightDelta = 0.0;
    double topDelta = 0.0;
    double bottomDelta = 0.0;

    switch (adjustHorizontalSide) {
    case IResizable::SIDE_NONE:
        break;
    case IResizable::SIDE_LEFT_OR_TOP:
        leftDelta = horizDelta;
        break;
    case IResizable::SIDE_RIGHT_OR_BOTTOM:
        rightDelta = -horizDelta;
        break;
    }

    switch (adjustVerticalSide) {
    case IResizable::SIDE_NONE:
        break;
    case IResizable::SIDE_LEFT_OR_TOP:
        topDelta = vertDelta;
        break;
    case IResizable::SIDE_RIGHT_OR_BOTTOM:
        bottomDelta = -vertDelta;
        break;
    }

    QPointF topLeftDelta(leftDelta, topDelta);
    QPointF bottomRightDelta(rightDelta, bottomDelta);
    setPosAndRect(pos, rect, topLeftDelta, bottomRightDelta);
}

void ObjectItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->getDiagramController()->startUpdateElement(m_object, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
    m_object->setPos(m_object->getPos() + delta);
    m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_object, m_diagramSceneModel->getDiagram(), false);
}

void ObjectItem::alignItemPositionToRaster(double rasterWidth, double rasterHeight)
{
    QPointF pos = m_object->getPos();
    QRectF rect = m_object->getRect();
    QPointF topLeft = pos + rect.topLeft();

    double leftDelta = qRound(topLeft.x() / rasterWidth) * rasterWidth - topLeft.x();
    double topDelta = qRound(topLeft.y() / rasterHeight) * rasterHeight - topLeft.y();
    QPointF topLeftDelta(leftDelta, topDelta);

    setPosAndRect(pos, rect, topLeftDelta, topLeftDelta);
}

bool ObjectItem::isSecondarySelected() const
{
    return m_secondarySelected;
}

void ObjectItem::setSecondarySelected(bool secondarySelected)
{
    if (m_secondarySelected != secondarySelected) {
        m_secondarySelected = secondarySelected;
        update();
    }
}

bool ObjectItem::isFocusSelected() const
{
    return m_focusSelected;
}

void ObjectItem::setFocusSelected(bool focusSelected)
{
    if (m_focusSelected != focusSelected) {
        m_focusSelected = focusSelected;
        update();
    }
}

QList<ILatchable::Latch> ObjectItem::getHorizontalLatches(ILatchable::Action action, bool grabbedItem) const
{
    Q_UNUSED(grabbedItem);

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

QList<ILatchable::Latch> ObjectItem::getVerticalLatches(ILatchable::Action action, bool grabbedItem) const
{
    Q_UNUSED(grabbedItem);

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

void ObjectItem::align(IAlignable::AlignType alignType, const QString &identifier)
{
    Q_UNUSED(identifier); // avoid warning in release mode

    // subclasses may support other identifiers than the standard ones.
    // but this implementation does not. So assert the names.
    switch (alignType) {
    case IAlignable::ALIGN_LEFT:
        QMT_CHECK(identifier == QStringLiteral("left"));
        m_diagramSceneModel->getDiagramSceneController()->alignLeft(m_object, m_diagramSceneModel->getSelectedElements(), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_RIGHT:
        QMT_CHECK(identifier == QStringLiteral("right"));
        m_diagramSceneModel->getDiagramSceneController()->alignRight(m_object, m_diagramSceneModel->getSelectedElements(), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_TOP:
        QMT_CHECK(identifier == QStringLiteral("top"));
        m_diagramSceneModel->getDiagramSceneController()->alignTop(m_object, m_diagramSceneModel->getSelectedElements(), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_BOTTOM:
        QMT_CHECK(identifier == QStringLiteral("bottom"));
        m_diagramSceneModel->getDiagramSceneController()->alignBottom(m_object, m_diagramSceneModel->getSelectedElements(), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_HCENTER:
        QMT_CHECK(identifier == QStringLiteral("center"));
        m_diagramSceneModel->getDiagramSceneController()->alignHCenter(m_object, m_diagramSceneModel->getSelectedElements(), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_VCENTER:
        QMT_CHECK(identifier == QStringLiteral("center"));
        m_diagramSceneModel->getDiagramSceneController()->alignVCenter(m_object, m_diagramSceneModel->getSelectedElements(), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_WIDTH:
        QMT_CHECK(identifier == QStringLiteral("width"));
        m_diagramSceneModel->getDiagramSceneController()->alignWidth(m_object, m_diagramSceneModel->getSelectedElements(),
                                                                      getMinimumSize(m_diagramSceneModel->getSelectedItems()), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_HEIGHT:
        QMT_CHECK(identifier == QStringLiteral("height"));
        m_diagramSceneModel->getDiagramSceneController()->alignHeight(m_object, m_diagramSceneModel->getSelectedElements(),
                                                                       getMinimumSize(m_diagramSceneModel->getSelectedItems()), m_diagramSceneModel->getDiagram());
        break;
    case IAlignable::ALIGN_SIZE:
        QMT_CHECK(identifier == QStringLiteral("size"));
        m_diagramSceneModel->getDiagramSceneController()->alignSize(m_object, m_diagramSceneModel->getSelectedElements(),
                                                                     getMinimumSize(m_diagramSceneModel->getSelectedItems()), m_diagramSceneModel->getDiagram());
        break;
    }
}

void ObjectItem::updateStereotypeIconDisplay()
{
    StereotypeDisplayVisitor stereotypeDisplayVisitor;
    stereotypeDisplayVisitor.setModelController(m_diagramSceneModel->getDiagramSceneController()->getModelController());
    stereotypeDisplayVisitor.setStereotypeController(m_diagramSceneModel->getStereotypeController());
    m_object->accept(&stereotypeDisplayVisitor);
    m_stereotypeIconId = stereotypeDisplayVisitor.getStereotypeIconId();
    m_shapeIconId = stereotypeDisplayVisitor.getShapeIconId();
    m_stereotypeIconDisplay = stereotypeDisplayVisitor.getStereotypeIconDisplay();
}

void ObjectItem::updateStereotypes(const QString &stereotypeIconId, StereotypeIcon::Display stereotypeDisplay, const Style *style)
{
    QList<QString> stereotypes = m_object->getStereotypes();
    if (!stereotypeIconId.isEmpty()
            && (stereotypeDisplay == StereotypeIcon::DISPLAY_DECORATION || stereotypeDisplay == StereotypeIcon::DISPLAY_ICON)) {
        stereotypes = m_diagramSceneModel->getStereotypeController()->filterStereotypesByIconId(stereotypeIconId, stereotypes);
    }
    if (!stereotypeIconId.isEmpty() && stereotypeDisplay == StereotypeIcon::DISPLAY_DECORATION) {
        if (!m_stereotypeIcon) {
            m_stereotypeIcon = new CustomIconItem(m_diagramSceneModel, this);
        }
        m_stereotypeIcon->setStereotypeIconId(stereotypeIconId);
        m_stereotypeIcon->setBaseSize(QSizeF(m_stereotypeIcon->getShapeWidth(), m_stereotypeIcon->getShapeHeight()));
        m_stereotypeIcon->setBrush(style->getFillBrush());
        m_stereotypeIcon->setPen(style->getInnerLinePen());
    } else if (m_stereotypeIcon) {
        m_stereotypeIcon->scene()->removeItem(m_stereotypeIcon);
        delete m_stereotypeIcon;
        m_stereotypeIcon = 0;
    }
    if (stereotypeDisplay != StereotypeIcon::DISPLAY_NONE && !stereotypes.isEmpty()) {
        if (!m_stereotypes) {
            m_stereotypes = new StereotypesItem(this);
        }
        m_stereotypes->setFont(style->getSmallFont());
        m_stereotypes->setBrush(style->getTextBrush());
        m_stereotypes->setStereotypes(stereotypes);
    } else if (m_stereotypes) {
        m_stereotypes->scene()->removeItem(m_stereotypes);
        delete m_stereotypes;
        m_stereotypes = 0;
    }
}

QSizeF ObjectItem::getStereotypeIconMinimumSize(const StereotypeIcon &stereotypeIcon, qreal minimumWidth, qreal minimumHeight) const
{
    Q_UNUSED(minimumWidth);

    qreal width = 0.0;
    qreal height = 0.0;
    if (stereotypeIcon.hasMinWidth() && !stereotypeIcon.hasMinHeight()) {
        width = stereotypeIcon.getMinWidth();
        if (stereotypeIcon.getSizeLock() == StereotypeIcon::LOCK_HEIGHT || stereotypeIcon.getSizeLock() == StereotypeIcon::LOCK_SIZE) {
            height = stereotypeIcon.getMinHeight();
        } else {
            height = width * stereotypeIcon.getHeight() / stereotypeIcon.getWidth();
        }
    } else if (!stereotypeIcon.hasMinWidth() && stereotypeIcon.hasMinHeight()) {
        height = stereotypeIcon.getMinHeight();
        if (stereotypeIcon.getSizeLock() == StereotypeIcon::LOCK_WIDTH || stereotypeIcon.getSizeLock() == StereotypeIcon::LOCK_SIZE) {
            width = stereotypeIcon.getMinWidth();
        } else {
            width = height * stereotypeIcon.getWidth() / stereotypeIcon.getHeight();
        }
    } else if (stereotypeIcon.hasMinWidth() && stereotypeIcon.hasMinHeight()) {
        if (stereotypeIcon.getSizeLock() == StereotypeIcon::LOCK_RATIO) {
            width = stereotypeIcon.getMinWidth();
            height = width * stereotypeIcon.getHeight() / stereotypeIcon.getWidth();
            if (height < stereotypeIcon.getMinHeight()) {
                height = stereotypeIcon.getMinHeight();
                width = height * stereotypeIcon.getWidth() / stereotypeIcon.getHeight();
                QMT_CHECK(width <= stereotypeIcon.getMinWidth());
            }
        } else {
            width = stereotypeIcon.getMinWidth();
            height = stereotypeIcon.getMinHeight();
        }
    } else {
        height = minimumHeight;
        width = height * stereotypeIcon.getWidth() / stereotypeIcon.getHeight();
    }
    return QSizeF(width, height);
}

void ObjectItem::updateDepth()
{
    setZValue(m_object->getDepth());
}

void ObjectItem::updateSelectionMarker(CustomIconItem *customIconItem)
{
    if (customIconItem) {
        StereotypeIcon stereotypeIcon = customIconItem->getStereotypeIcon();
        ResizeFlags resizeFlags = RESIZE_UNLOCKED;
        switch (stereotypeIcon.getSizeLock()) {
        case StereotypeIcon::LOCK_NONE:
            resizeFlags = RESIZE_UNLOCKED;
            break;
        case StereotypeIcon::LOCK_WIDTH:
            resizeFlags = RESIZE_LOCKED_WIDTH;
            break;
        case StereotypeIcon::LOCK_HEIGHT:
            resizeFlags = RESIZE_LOCKED_HEIGHT;
            break;
        case StereotypeIcon::LOCK_SIZE:
            resizeFlags = RESIZE_LOCKED_SIZE;
            break;
        case StereotypeIcon::LOCK_RATIO:
            resizeFlags = RESIZE_LOCKED_RATIO;
            break;
        }
        updateSelectionMarker(resizeFlags);
    } else {
        updateSelectionMarker(RESIZE_UNLOCKED);
    }
}

void ObjectItem::updateSelectionMarker(ResizeFlags resizeFlags)
{
    if ((isSelected() || isSecondarySelected()) && resizeFlags != RESIZE_LOCKED_SIZE) {
        if (!m_selectionMarker) {
            m_selectionMarker = new RectangularSelectionItem(this, this);
        }
        switch (resizeFlags) {
        case RESIZE_UNLOCKED:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FREEDOM_ANY);
            break;
        case RESIZE_LOCKED_SIZE:
            QMT_CHECK(false);
            break;
        case RESIZE_LOCKED_WIDTH:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FREEDOM_VERTICAL_ONLY);
            break;
        case RESIZE_LOCKED_HEIGHT:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FREEDOM_HORIZONTAL_ONLY);
            break;
        case RESIZE_LOCKED_RATIO:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FREEDOM_KEEP_RATIO);
            break;
        }
        m_selectionMarker->setSecondarySelected(isSelected() ? false : isSecondarySelected());
        m_selectionMarker->setZValue(SELECTION_MARKER_ZVALUE);
    } else if (m_selectionMarker) {
        if (m_selectionMarker->scene()) {
            m_selectionMarker->scene()->removeItem(m_selectionMarker);
        }
        delete m_selectionMarker;
        m_selectionMarker = 0;
    }
}

void ObjectItem::updateSelectionMarkerGeometry(const QRectF &objectRect)
{
    if (m_selectionMarker) {
        m_selectionMarker->setRect(objectRect);
    }
}

void ObjectItem::updateAlignmentButtons()
{
    if (isFocusSelected() && m_diagramSceneModel->hasMultiObjectsSelection()) {
        if (!m_horizontalAlignButtons && scene()) {
            m_horizontalAlignButtons = new AlignButtonsItem(this, 0);
            m_horizontalAlignButtons->setZValue(ALIGN_BUTTONS_ZVALUE);
            scene()->addItem(m_horizontalAlignButtons);
        }

        if (!m_verticalAlignButtons && scene()) {
            m_verticalAlignButtons = new AlignButtonsItem(this, 0);
            m_verticalAlignButtons->setZValue(ALIGN_BUTTONS_ZVALUE);
            scene()->addItem(m_verticalAlignButtons);
        }
    } else {
        if (m_horizontalAlignButtons) {
            if (m_horizontalAlignButtons->scene()) {
                m_horizontalAlignButtons->scene()->removeItem(m_horizontalAlignButtons);
            }
            delete m_horizontalAlignButtons;
            m_horizontalAlignButtons = 0;
        }
        if (m_verticalAlignButtons) {
            if (m_verticalAlignButtons->scene()) {
                m_verticalAlignButtons->scene()->removeItem(m_verticalAlignButtons);
            }
            delete m_verticalAlignButtons;
            m_verticalAlignButtons = 0;
        }
    }
}

void ObjectItem::updateAlignmentButtonsGeometry(const QRectF &objectRect)
{
    if (m_horizontalAlignButtons) {
        m_horizontalAlignButtons->clear();
        m_horizontalAlignButtons->setPos(mapToScene(QPointF(0.0, objectRect.top() - AlignButtonsItem::NORMAL_BUTTON_HEIGHT - AlignButtonsItem::VERTICAL_DISTANCE_TO_OBEJCT)));
        foreach (const ILatchable::Latch &latch, getHorizontalLatches(ILatchable::MOVE, true)) {
            m_horizontalAlignButtons->addButton(translateLatchTypeToAlignType(latch.m_latchType), latch.m_identifier, mapFromScene(QPointF(latch.m_pos, 0.0)).x());
        }
    }
    if (m_verticalAlignButtons) {
        m_verticalAlignButtons->clear();
        m_verticalAlignButtons->setPos(mapToScene(QPointF(objectRect.left() - AlignButtonsItem::NORMAL_BUTTON_WIDTH - AlignButtonsItem::HORIZONTAL_DISTANCE_TO_OBJECT, 0.0)));
        foreach (const ILatchable::Latch &latch, getVerticalLatches(ILatchable::MOVE, true)) {
            m_verticalAlignButtons->addButton(translateLatchTypeToAlignType(latch.m_latchType), latch.m_identifier, mapFromScene(QPointF(0.0, latch.m_pos)).y());
        }
    }
}

IAlignable::AlignType ObjectItem::translateLatchTypeToAlignType(ILatchable::LatchType latchType)
{
    IAlignable::AlignType alignType = IAlignable::ALIGN_LEFT;
    switch (latchType) {
    case ILatchable::LEFT:
        alignType = IAlignable::ALIGN_LEFT;
        break;
    case ILatchable::TOP:
        alignType = IAlignable::ALIGN_TOP;
        break;
    case ILatchable::RIGHT:
        alignType = IAlignable::ALIGN_RIGHT;
        break;
    case ILatchable::BOTTOM:
        alignType = IAlignable::ALIGN_BOTTOM;
        break;
    case ILatchable::HCENTER:
        alignType = IAlignable::ALIGN_HCENTER;
        break;
    case ILatchable::VCENTER:
        alignType = IAlignable::ALIGN_VCENTER;
        break;
    case ILatchable::NONE:
        QMT_CHECK(false);
        break;
    }
    return alignType;
}

const Style *ObjectItem::getAdaptedStyle(const QString &stereotypeIconId)
{
    QList<const DObject *> collidingObjects;
    foreach (const QGraphicsItem *item, m_diagramSceneModel->collectCollidingObjectItems(this, DiagramSceneModel::COLLIDING_ITEMS)) {
        if (const ObjectItem *objectItem = dynamic_cast<const ObjectItem *>(item)) {
            collidingObjects.append(objectItem->getObject());
        }
    }
    QColor baseColor;
    if (!stereotypeIconId.isEmpty()) {
        StereotypeIcon stereotypeIcon = m_diagramSceneModel->getStereotypeController()->findStereotypeIcon(stereotypeIconId);
        baseColor = stereotypeIcon.getBaseColor();
    }
    return m_diagramSceneModel->getStyleController()->adaptObjectStyle(
                StyledObject(getObject(),
                             ObjectVisuals(
                                 getObject()->getVisualPrimaryRole(),
                                 getObject()->getVisualSecondaryRole(),
                                 getObject()->isVisualEmphasized(),
                                 baseColor,
                                 getObject()->getDepth()),
                             collidingObjects));
}

bool ObjectItem::showContext() const
{
    bool showContext = !m_object->getContext().isEmpty();
    if (showContext) {
        // TODO Because of this algorithm adding, moving, removing of one item need to update() all colliding items as well
        QMT_CHECK(getObject()->getModelUid().isValid());
        MObject *mobject = m_diagramSceneModel->getDiagramController()->getModelController()->findObject(getObject()->getModelUid());
        QMT_CHECK(mobject);
        MObject *owner = mobject->getOwner();
        if (owner) {
            foreach (QGraphicsItem *item, m_diagramSceneModel->collectCollidingObjectItems(this, DiagramSceneModel::COLLIDING_OUTER_ITEMS)) {
                if (ObjectItem *objectItem = dynamic_cast<ObjectItem *>(item)) {
                    if (objectItem->getObject()->getModelUid().isValid() && objectItem->getObject()->getModelUid() == owner->getUid()) {
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
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
    }
    if (event->button() == Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, QPointF(0.0, 0.0));
    }
}

void ObjectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
    }
}

void ObjectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton)) {
            m_diagramSceneModel->alignSelectedItemsPositionOnRaster();
        }
    }
}

void ObjectItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        m_diagramSceneModel->onDoubleClickedItem(this);
    }
}

void ObjectItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;

    bool addSeparator = false;
    if (getDiagramSceneModel()->getDiagramSceneController()->getElementTasks()->hasDiagram(m_object, m_diagramSceneModel->getDiagram())) {
        menu.addAction(new ContextMenuAction(QObject::tr("Open Diagram"), QStringLiteral("openDiagram"), &menu));
        addSeparator = true;
    } else if (getDiagramSceneModel()->getDiagramSceneController()->getElementTasks()->mayCreateDiagram(m_object, m_diagramSceneModel->getDiagram())) {
        menu.addAction(new ContextMenuAction(QObject::tr("Create Diagram"), QStringLiteral("createDiagram"), &menu));
        addSeparator = true;
    }
    if (extendContextMenu(&menu)) {
        addSeparator = true;
    }
    if (addSeparator) {
        menu.addSeparator();
    }
    menu.addAction(new ContextMenuAction(QObject::tr("Remove"), QStringLiteral("remove"), QKeySequence(QKeySequence::Delete), &menu));
    menu.addAction(new ContextMenuAction(QObject::tr("Delete"), QStringLiteral("delete"), QKeySequence(Qt::CTRL + Qt::Key_D), &menu));
    //menu.addAction(new ContextMenuAction(QObject::tr("Select in Model Tree"), QStringLiteral("selectInModelTree"), &menu));
    QMenu alignMenu;
    alignMenu.setTitle(QObject::tr("Align Objects"));
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Align Left"), QStringLiteral("alignLeft"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Center Vertically"), QStringLiteral("centerVertically"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Align Right"), QStringLiteral("alignRight"), &alignMenu));
    alignMenu.addSeparator();
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Align Top"), QStringLiteral("alignTop"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Center Horizontally"), QStringLiteral("centerHorizontally"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Align Bottom"), QStringLiteral("alignBottom"), &alignMenu));
    alignMenu.addSeparator();
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Same Width"), QStringLiteral("sameWidth"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Same Height"), QStringLiteral("sameHeight"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(QObject::tr("Same Size"), QStringLiteral("sameSize"), &alignMenu));
    alignMenu.setEnabled(m_diagramSceneModel->hasMultiObjectsSelection());
    menu.addMenu(&alignMenu);

    QAction *selectedAction = menu.exec(event->screenPos());
    if (selectedAction) {
        if (!handleSelectedContextMenuAction(selectedAction)) {
            ContextMenuAction *action = dynamic_cast<ContextMenuAction *>(selectedAction);
            QMT_CHECK(action);
            if (action->getId() == QStringLiteral("openDiagram")) {
                m_diagramSceneModel->getDiagramSceneController()->getElementTasks()->openDiagram(m_object, m_diagramSceneModel->getDiagram());
            } else if (action->getId() == QStringLiteral("createDiagram")) {
                m_diagramSceneModel->getDiagramSceneController()->getElementTasks()->createAndOpenDiagram(m_object, m_diagramSceneModel->getDiagram());
            } else if (action->getId() == QStringLiteral("remove")) {
                DSelection selection = m_diagramSceneModel->getSelectedElements();
                if (selection.isEmpty()) {
                    selection.append(m_object->getUid(), m_diagramSceneModel->getDiagram()->getUid());
                }
                m_diagramSceneModel->getDiagramController()->deleteElements(selection, m_diagramSceneModel->getDiagram());
            } else if (action->getId() == QStringLiteral("delete")) {
                DSelection selection = m_diagramSceneModel->getSelectedElements();
                if (selection.isEmpty()) {
                    selection.append(m_object->getUid(), m_diagramSceneModel->getDiagram()->getUid());
                }
                m_diagramSceneModel->getDiagramSceneController()->deleteFromDiagram(selection, m_diagramSceneModel->getDiagram());
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
    QSizeF minimumSize(0.0, 0.0);
    foreach (QGraphicsItem *item, items) {
        if (IResizable *resizable = dynamic_cast<IResizable *>(item)) {
            QSizeF size = resizable->getMinimumSize();
            if (size.width() > minimumSize.width()) {
                minimumSize.setWidth(size.width());
            }
            if (size.height() > minimumSize.height()) {
                minimumSize.setHeight(size.height());
            }
        }
    }
    return minimumSize;
}

}
