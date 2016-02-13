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

#include "objectitem.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/items/stereotypedisplayvisitor.h"
#include "qmt/diagram_scene/parts/alignbuttonsitem.h"
#include "qmt/diagram_scene/parts/editabletextitem.h"
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
#include <QTextDocument>

namespace qmt {

ObjectItem::ObjectItem(DObject *object, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_object(object),
      m_diagramSceneModel(diagramSceneModel)
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

QPointF ObjectItem::pos() const
{
    return m_object->pos();
}

QRectF ObjectItem::rect() const
{
    return m_object->rect();
}

void ObjectItem::setPosAndRect(const QPointF &originalPos, const QRectF &originalRect, const QPointF &topLeftDelta,
                               const QPointF &bottomRightDelta)
{
    QPointF newPos = originalPos;
    QRectF newRect = originalRect;
    GeometryUtilities::adjustPosAndRect(&newPos, &newRect, topLeftDelta, bottomRightDelta, QPointF(0.5, 0.5));
    if (newPos != m_object->pos() || newRect != m_object->rect()) {
        m_diagramSceneModel->diagramController()->startUpdateElement(m_object, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
        m_object->setPos(newPos);
        if (newRect.size() != m_object->rect().size())
            m_object->setAutoSized(false);
        m_object->setRect(newRect);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_object, m_diagramSceneModel->diagram(), false);
    }
}

void ObjectItem::alignItemSizeToRaster(IResizable::Side adjustHorizontalSide, IResizable::Side adjustVerticalSide,
                                       double rasterWidth, double rasterHeight)
{
    QPointF pos = m_object->pos();
    QRectF rect = m_object->rect();

    double horizDelta = rect.width() - qRound(rect.width() / rasterWidth) * rasterWidth;
    double vertDelta = rect.height() - qRound(rect.height() / rasterHeight) * rasterHeight;

    // make sure the new size is at least the minimum size
    QSizeF minimumSize = this->minimumSize();
    while (rect.width() + horizDelta < minimumSize.width())
        horizDelta += rasterWidth;
    while (rect.height() + vertDelta < minimumSize.height())
        vertDelta += rasterHeight;

    double leftDelta = 0.0;
    double rightDelta = 0.0;
    double topDelta = 0.0;
    double bottomDelta = 0.0;

    switch (adjustHorizontalSide) {
    case IResizable::SideNone:
        break;
    case IResizable::SideLeftOrTop:
        leftDelta = horizDelta;
        break;
    case IResizable::SideRightOrBottom:
        rightDelta = -horizDelta;
        break;
    }

    switch (adjustVerticalSide) {
    case IResizable::SideNone:
        break;
    case IResizable::SideLeftOrTop:
        topDelta = vertDelta;
        break;
    case IResizable::SideRightOrBottom:
        bottomDelta = -vertDelta;
        break;
    }

    QPointF topLeftDelta(leftDelta, topDelta);
    QPointF bottomRightDelta(rightDelta, bottomDelta);
    setPosAndRect(pos, rect, topLeftDelta, bottomRightDelta);
}

void ObjectItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->diagramController()->startUpdateElement(m_object, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
    m_object->setPos(m_object->pos() + delta);
    m_diagramSceneModel->diagramController()->finishUpdateElement(m_object, m_diagramSceneModel->diagram(), false);
}

void ObjectItem::alignItemPositionToRaster(double rasterWidth, double rasterHeight)
{
    QPointF pos = m_object->pos();
    QRectF rect = m_object->rect();
    QPointF topLeft = pos + rect.topLeft();

    double leftDelta = qRound(topLeft.x() / rasterWidth) * rasterWidth - topLeft.x();
    double topDelta = qRound(topLeft.y() / rasterHeight) * rasterHeight - topLeft.y();
    QPointF topLeftDelta(leftDelta, topDelta);

    setPosAndRect(pos, rect, topLeftDelta, topLeftDelta);
}

bool ObjectItem::isSecondarySelected() const
{
    return m_isSecondarySelected;
}

void ObjectItem::setSecondarySelected(bool secondarySelected)
{
    if (m_isSecondarySelected != secondarySelected) {
        m_isSecondarySelected = secondarySelected;
        update();
    }
}

bool ObjectItem::isFocusSelected() const
{
    return m_isFocusSelected;
}

void ObjectItem::setFocusSelected(bool focusSelected)
{
    if (m_isFocusSelected != focusSelected) {
        m_isFocusSelected = focusSelected;
        update();
    }
}

ILatchable::Action ObjectItem::horizontalLatchAction() const
{
    switch (m_selectionMarker->activeHandle()) {
    case RectangularSelectionItem::HandleTopLeft:
    case RectangularSelectionItem::HandleLeft:
    case RectangularSelectionItem::HandleBottomLeft:
        return ResizeLeft;
    case RectangularSelectionItem::HandleTopRight:
    case RectangularSelectionItem::HandleRight:
    case RectangularSelectionItem::HandleBottomRight:
        return ResizeRight;
    case RectangularSelectionItem::HandleTop:
    case RectangularSelectionItem::HandleBottom:
        return Move; // TODO should be ActionNone
    case RectangularSelectionItem::HandleNone:
        return Move;
    }
    // avoid warning from MSVC compiler
    QMT_CHECK(false);
    return Move;
}

ILatchable::Action ObjectItem::verticalLatchAction() const
{
    switch (m_selectionMarker->activeHandle()) {
    case RectangularSelectionItem::HandleTopLeft:
    case RectangularSelectionItem::HandleTop:
    case RectangularSelectionItem::HandleTopRight:
        return ResizeTop;
    case RectangularSelectionItem::HandleBottomLeft:
    case RectangularSelectionItem::HandleBottom:
    case RectangularSelectionItem::HandleBottomRight:
        return ResizeBottom;
    case RectangularSelectionItem::HandleLeft:
    case RectangularSelectionItem::HandleRight:
        return Move; // TODO should be ActionNone
    case RectangularSelectionItem::HandleNone:
        return Move;
    }
    // avoid warning from MSVC compiler
    QMT_CHECK(false);
    return Move;
}

QList<ILatchable::Latch> ObjectItem::horizontalLatches(ILatchable::Action action, bool grabbedItem) const
{
    Q_UNUSED(grabbedItem);

    QRectF rect = mapRectToScene(this->rect());
    QList<ILatchable::Latch> result;
    switch (action) {
    case ILatchable::Move:
        result << ILatchable::Latch(ILatchable::Left, rect.left(), rect.top(), rect.bottom(), QStringLiteral("left"))
               << ILatchable::Latch(ILatchable::Hcenter, rect.center().x(), rect.top(), rect.bottom(), QStringLiteral("center"))
               << ILatchable::Latch(ILatchable::Right, rect.right(), rect.top(), rect.bottom(), QStringLiteral("right"));
        break;
    case ILatchable::ResizeLeft:
        result << ILatchable::Latch(ILatchable::Left, rect.left(), rect.top(), rect.bottom(), QStringLiteral("left"));
        break;
    case ILatchable::ResizeTop:
        QMT_CHECK(false);
        break;
    case ILatchable::ResizeRight:
        result << ILatchable::Latch(ILatchable::Right, rect.right(), rect.top(), rect.bottom(), QStringLiteral("right"));
        break;
    case ILatchable::ResizeBottom:
        QMT_CHECK(false);
        break;
    }
    return result;
}

QList<ILatchable::Latch> ObjectItem::verticalLatches(ILatchable::Action action, bool grabbedItem) const
{
    Q_UNUSED(grabbedItem);

    QRectF rect = mapRectToScene(this->rect());
    QList<ILatchable::Latch> result;
    switch (action) {
    case ILatchable::Move:
        result << ILatchable::Latch(ILatchable::Top, rect.top(), rect.left(), rect.right(), QStringLiteral("top"))
               << ILatchable::Latch(ILatchable::Vcenter, rect.center().y(), rect.left(), rect.right(), QStringLiteral("center"))
               << ILatchable::Latch(ILatchable::Bottom, rect.bottom(), rect.left(), rect.right(), QStringLiteral("bottom"));
        break;
    case ILatchable::ResizeLeft:
        QMT_CHECK(false);
        break;
    case ILatchable::ResizeTop:
        result << ILatchable::Latch(ILatchable::Top, rect.top(), rect.left(), rect.right(), QStringLiteral("top"));
        break;
    case ILatchable::ResizeRight:
        QMT_CHECK(false);
        break;
    case ILatchable::ResizeBottom:
        result << ILatchable::Latch(ILatchable::Bottom, rect.bottom(), rect.left(), rect.right(), QStringLiteral("bottom"));
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
    case IAlignable::AlignLeft:
        QMT_CHECK(identifier == QStringLiteral("left"));
        m_diagramSceneModel->diagramSceneController()->alignLeft(m_object, m_diagramSceneModel->selectedElements(),
                                                                 m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignRight:
        QMT_CHECK(identifier == QStringLiteral("right"));
        m_diagramSceneModel->diagramSceneController()->alignRight(m_object, m_diagramSceneModel->selectedElements(),
                                                                  m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignTop:
        QMT_CHECK(identifier == QStringLiteral("top"));
        m_diagramSceneModel->diagramSceneController()->alignTop(m_object, m_diagramSceneModel->selectedElements(),
                                                                m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignBottom:
        QMT_CHECK(identifier == QStringLiteral("bottom"));
        m_diagramSceneModel->diagramSceneController()->alignBottom(m_object, m_diagramSceneModel->selectedElements(),
                                                                   m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignHcenter:
        QMT_CHECK(identifier == QStringLiteral("center"));
        m_diagramSceneModel->diagramSceneController()->alignHCenter(m_object, m_diagramSceneModel->selectedElements(),
                                                                    m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignVcenter:
        QMT_CHECK(identifier == QStringLiteral("center"));
        m_diagramSceneModel->diagramSceneController()->alignVCenter(m_object, m_diagramSceneModel->selectedElements(),
                                                                    m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignWidth:
        QMT_CHECK(identifier == QStringLiteral("width"));
        m_diagramSceneModel->diagramSceneController()->alignWidth(m_object, m_diagramSceneModel->selectedElements(),
                                                                  minimumSize(m_diagramSceneModel->selectedItems()),
                                                                  m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignHeight:
        QMT_CHECK(identifier == QStringLiteral("height"));
        m_diagramSceneModel->diagramSceneController()->alignHeight(m_object, m_diagramSceneModel->selectedElements(),
                                                                   minimumSize(m_diagramSceneModel->selectedItems()),
                                                                   m_diagramSceneModel->diagram());
        break;
    case IAlignable::AlignSize:
        QMT_CHECK(identifier == QStringLiteral("size"));
        m_diagramSceneModel->diagramSceneController()->alignSize(m_object, m_diagramSceneModel->selectedElements(),
                                                                 minimumSize(m_diagramSceneModel->selectedItems()),
                                                                 m_diagramSceneModel->diagram());
        break;
    }
}

bool ObjectItem::isEditable() const
{
    return true;
}

void ObjectItem::edit()
{
    // TODO if name is initial name ("New Class" etc) select all text
    if (m_nameItem)
        m_nameItem->setFocus();
}

void ObjectItem::updateStereotypeIconDisplay()
{
    StereotypeDisplayVisitor stereotypeDisplayVisitor;
    stereotypeDisplayVisitor.setModelController(m_diagramSceneModel->diagramSceneController()->modelController());
    stereotypeDisplayVisitor.setStereotypeController(m_diagramSceneModel->stereotypeController());
    m_object->accept(&stereotypeDisplayVisitor);
    m_stereotypeIconId = stereotypeDisplayVisitor.stereotypeIconId();
    m_shapeIconId = stereotypeDisplayVisitor.shapeIconId();
    m_stereotypeIconDisplay = stereotypeDisplayVisitor.stereotypeIconDisplay();
}

void ObjectItem::updateStereotypes(const QString &stereotypeIconId, StereotypeIcon::Display stereotypeDisplay,
                                   const Style *style)
{
    QList<QString> stereotypes = m_object->stereotypes();
    if (!stereotypeIconId.isEmpty()
            && (stereotypeDisplay == StereotypeIcon::DisplayDecoration || stereotypeDisplay == StereotypeIcon::DisplayIcon)) {
        stereotypes = m_diagramSceneModel->stereotypeController()->filterStereotypesByIconId(stereotypeIconId, stereotypes);
    }
    if (!stereotypeIconId.isEmpty() && stereotypeDisplay == StereotypeIcon::DisplayDecoration) {
        if (!m_stereotypeIcon)
            m_stereotypeIcon = new CustomIconItem(m_diagramSceneModel, this);
        m_stereotypeIcon->setStereotypeIconId(stereotypeIconId);
        m_stereotypeIcon->setBaseSize(QSizeF(m_stereotypeIcon->shapeWidth(), m_stereotypeIcon->shapeHeight()));
        m_stereotypeIcon->setBrush(style->fillBrush());
        m_stereotypeIcon->setPen(style->innerLinePen());
    } else if (m_stereotypeIcon) {
        m_stereotypeIcon->scene()->removeItem(m_stereotypeIcon);
        delete m_stereotypeIcon;
        m_stereotypeIcon = 0;
    }
    if (stereotypeDisplay != StereotypeIcon::DisplayNone && !stereotypes.isEmpty()) {
        if (!m_stereotypes)
            m_stereotypes = new StereotypesItem(this);
        m_stereotypes->setFont(style->smallFont());
        m_stereotypes->setBrush(style->textBrush());
        m_stereotypes->setStereotypes(stereotypes);
    } else if (m_stereotypes) {
        m_stereotypes->scene()->removeItem(m_stereotypes);
        delete m_stereotypes;
        m_stereotypes = 0;
    }
}

QSizeF ObjectItem::stereotypeIconMinimumSize(const StereotypeIcon &stereotypeIcon,
                                             qreal minimumWidth, qreal minimumHeight) const
{
    Q_UNUSED(minimumWidth);

    qreal width = 0.0;
    qreal height = 0.0;
    if (stereotypeIcon.hasMinWidth() && !stereotypeIcon.hasMinHeight()) {
        width = stereotypeIcon.minWidth();
        if (stereotypeIcon.sizeLock() == StereotypeIcon::LockHeight || stereotypeIcon.sizeLock() == StereotypeIcon::LockSize)
            height = stereotypeIcon.minHeight();
        else
            height = width * stereotypeIcon.height() / stereotypeIcon.width();
    } else if (!stereotypeIcon.hasMinWidth() && stereotypeIcon.hasMinHeight()) {
        height = stereotypeIcon.minHeight();
        if (stereotypeIcon.sizeLock() == StereotypeIcon::LockWidth || stereotypeIcon.sizeLock() == StereotypeIcon::LockSize)
            width = stereotypeIcon.minWidth();
        else
            width = height * stereotypeIcon.width() / stereotypeIcon.height();
    } else if (stereotypeIcon.hasMinWidth() && stereotypeIcon.hasMinHeight()) {
        if (stereotypeIcon.sizeLock() == StereotypeIcon::LockRatio) {
            width = stereotypeIcon.minWidth();
            height = width * stereotypeIcon.height() / stereotypeIcon.width();
            if (height < stereotypeIcon.minHeight()) {
                height = stereotypeIcon.minHeight();
                width = height * stereotypeIcon.width() / stereotypeIcon.height();
                QMT_CHECK(width <= stereotypeIcon.minWidth());
            }
        } else {
            width = stereotypeIcon.minWidth();
            height = stereotypeIcon.minHeight();
        }
    } else {
        height = minimumHeight;
        width = height * stereotypeIcon.width() / stereotypeIcon.height();
    }
    return QSizeF(width, height);
}

void ObjectItem::updateNameItem(const Style *style)
{
    if (!m_nameItem) {
        m_nameItem = new EditableTextItem(this);
        m_nameItem->setShowFocus(true);
        m_nameItem->setFilterReturnKey(true);
        m_nameItem->setFilterTabKey(true);
        QObject::connect(m_nameItem->document(), &QTextDocument::contentsChanged, m_nameItem,
                         [=]() { this->setFromDisplayName(m_nameItem->toPlainText()); });
        QObject::connect(m_nameItem, &EditableTextItem::returnKeyPressed, m_nameItem,
                         [=]() { this->m_nameItem->clearFocus(); });
    }
    if (style->headerFont() != m_nameItem->font())
        m_nameItem->setFont(style->headerFont());
    if (style->textBrush().color() != m_nameItem->defaultTextColor())
        m_nameItem->setDefaultTextColor(style->textBrush().color());
    if (!m_nameItem->hasFocus()) {
        QString name = buildDisplayName();
        if (name != m_nameItem->toPlainText())
            m_nameItem->setPlainText(name);
    }
}

QString ObjectItem::buildDisplayName() const
{
    return m_object->name();
}

void ObjectItem::setFromDisplayName(const QString &displayName)
{
    setObjectName(displayName);
}

void ObjectItem::setObjectName(const QString &objectName)
{
    ModelController *modelController = m_diagramSceneModel->diagramSceneController()->modelController();
    MObject *mobject = modelController->findObject(m_object->modelUid());
    if (mobject && objectName != mobject->name()) {
        modelController->startUpdateObject(mobject);
        mobject->setName(objectName);
        modelController->finishUpdateObject(mobject, false);
    }
}

void ObjectItem::updateDepth()
{
    setZValue(m_object->depth());
}

void ObjectItem::updateSelectionMarker(CustomIconItem *customIconItem)
{
    if (customIconItem) {
        StereotypeIcon stereotypeIcon = customIconItem->stereotypeIcon();
        ResizeFlags resizeFlags = ResizeUnlocked;
        switch (stereotypeIcon.sizeLock()) {
        case StereotypeIcon::LockNone:
            resizeFlags = ResizeUnlocked;
            break;
        case StereotypeIcon::LockWidth:
            resizeFlags = ResizeLockedWidth;
            break;
        case StereotypeIcon::LockHeight:
            resizeFlags = ResizeLockedHeight;
            break;
        case StereotypeIcon::LockSize:
            resizeFlags = ResizeLockedSize;
            break;
        case StereotypeIcon::LockRatio:
            resizeFlags = ResizeLockedRatio;
            break;
        }
        updateSelectionMarker(resizeFlags);
    } else {
        updateSelectionMarker(ResizeUnlocked);
    }
}

void ObjectItem::updateSelectionMarker(ResizeFlags resizeFlags)
{
    if ((isSelected() || isSecondarySelected()) && resizeFlags != ResizeLockedSize) {
        if (!m_selectionMarker)
            m_selectionMarker = new RectangularSelectionItem(this, this);
        switch (resizeFlags) {
        case ResizeUnlocked:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FreedomAny);
            break;
        case ResizeLockedSize:
            QMT_CHECK(false);
            break;
        case ResizeLockedWidth:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FreedomVerticalOnly);
            break;
        case ResizeLockedHeight:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FreedomHorizontalOnly);
            break;
        case ResizeLockedRatio:
            m_selectionMarker->setFreedom(RectangularSelectionItem::FreedomKeepRatio);
            break;
        }
        m_selectionMarker->setSecondarySelected(isSelected() ? false : isSecondarySelected());
        m_selectionMarker->setZValue(SELECTION_MARKER_ZVALUE);
    } else if (m_selectionMarker) {
        if (m_selectionMarker->scene())
            m_selectionMarker->scene()->removeItem(m_selectionMarker);
        delete m_selectionMarker;
        m_selectionMarker = 0;
    }
}

void ObjectItem::updateSelectionMarkerGeometry(const QRectF &objectRect)
{
    if (m_selectionMarker)
        m_selectionMarker->setRect(objectRect);
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
            if (m_horizontalAlignButtons->scene())
                m_horizontalAlignButtons->scene()->removeItem(m_horizontalAlignButtons);
            delete m_horizontalAlignButtons;
            m_horizontalAlignButtons = 0;
        }
        if (m_verticalAlignButtons) {
            if (m_verticalAlignButtons->scene())
                m_verticalAlignButtons->scene()->removeItem(m_verticalAlignButtons);
            delete m_verticalAlignButtons;
            m_verticalAlignButtons = 0;
        }
    }
}

void ObjectItem::updateAlignmentButtonsGeometry(const QRectF &objectRect)
{
    if (m_horizontalAlignButtons) {
        m_horizontalAlignButtons->clear();
        m_horizontalAlignButtons->setPos(mapToScene(QPointF(0.0, objectRect.top() - AlignButtonsItem::NormalButtonHeight - AlignButtonsItem::VerticalDistanceToObejct)));
        foreach (const ILatchable::Latch &latch, horizontalLatches(ILatchable::Move, true))
            m_horizontalAlignButtons->addButton(translateLatchTypeToAlignType(latch.m_latchType), latch.m_identifier, mapFromScene(QPointF(latch.m_pos, 0.0)).x());
    }
    if (m_verticalAlignButtons) {
        m_verticalAlignButtons->clear();
        m_verticalAlignButtons->setPos(mapToScene(QPointF(objectRect.left() - AlignButtonsItem::NormalButtonWidth - AlignButtonsItem::HorizontalDistanceToObject, 0.0)));
        foreach (const ILatchable::Latch &latch, verticalLatches(ILatchable::Move, true))
            m_verticalAlignButtons->addButton(translateLatchTypeToAlignType(latch.m_latchType), latch.m_identifier, mapFromScene(QPointF(0.0, latch.m_pos)).y());
    }
}

IAlignable::AlignType ObjectItem::translateLatchTypeToAlignType(ILatchable::LatchType latchType)
{
    IAlignable::AlignType alignType = IAlignable::AlignLeft;
    switch (latchType) {
    case ILatchable::Left:
        alignType = IAlignable::AlignLeft;
        break;
    case ILatchable::Top:
        alignType = IAlignable::AlignTop;
        break;
    case ILatchable::Right:
        alignType = IAlignable::AlignRight;
        break;
    case ILatchable::Bottom:
        alignType = IAlignable::AlignBottom;
        break;
    case ILatchable::Hcenter:
        alignType = IAlignable::AlignHcenter;
        break;
    case ILatchable::Vcenter:
        alignType = IAlignable::AlignVcenter;
        break;
    case ILatchable::None:
        QMT_CHECK(false);
        break;
    }
    return alignType;
}

const Style *ObjectItem::adaptedStyle(const QString &stereotypeIconId)
{
    QList<const DObject *> collidingObjects;
    foreach (const QGraphicsItem *item, m_diagramSceneModel->collectCollidingObjectItems(this, DiagramSceneModel::CollidingItems)) {
        if (auto objectItem = dynamic_cast<const ObjectItem *>(item))
            collidingObjects.append(objectItem->object());
    }
    QColor baseColor;
    if (!stereotypeIconId.isEmpty()) {
        StereotypeIcon stereotypeIcon = m_diagramSceneModel->stereotypeController()->findStereotypeIcon(stereotypeIconId);
        baseColor = stereotypeIcon.baseColor();
    }
    return m_diagramSceneModel->styleController()->adaptObjectStyle(
                StyledObject(object(),
                             ObjectVisuals(
                                 object()->visualPrimaryRole(),
                                 object()->visualSecondaryRole(),
                                 object()->isVisualEmphasized(),
                                 baseColor,
                                 object()->depth()),
                             collidingObjects));
}

bool ObjectItem::showContext() const
{
    bool showContext = !m_object->context().isEmpty();
    if (showContext) {
        // TODO Because of this algorithm adding, moving, removing of one item need to update() all colliding items as well
        QMT_CHECK(object()->modelUid().isValid());
        MObject *mobject = m_diagramSceneModel->diagramController()->modelController()->findObject(object()->modelUid());
        QMT_CHECK(mobject);
        MObject *owner = mobject->owner();
        if (owner) {
            foreach (QGraphicsItem *item, m_diagramSceneModel->collectCollidingObjectItems(this, DiagramSceneModel::CollidingOuterItems)) {
                if (auto objectItem = dynamic_cast<ObjectItem *>(item)) {
                    if (objectItem->object()->modelUid().isValid() && objectItem->object()->modelUid() == owner->uid()) {
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
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
    if (event->button() == Qt::LeftButton)
        m_diagramSceneModel->moveSelectedItems(this, QPointF(0.0, 0.0));
}

void ObjectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
}

void ObjectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton))
            m_diagramSceneModel->alignSelectedItemsPositionOnRaster();
    }
}

void ObjectItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        m_diagramSceneModel->onDoubleClickedItem(this);
}

void ObjectItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;

    bool addSeparator = false;
    if (diagramSceneModel()->diagramSceneController()->elementTasks()->hasDiagram(m_object, m_diagramSceneModel->diagram())) {
        menu.addAction(new ContextMenuAction(tr("Open Diagram"), QStringLiteral("openDiagram"), &menu));
        addSeparator = true;
    } else if (diagramSceneModel()->diagramSceneController()->elementTasks()->mayCreateDiagram(m_object, m_diagramSceneModel->diagram())) {
        menu.addAction(new ContextMenuAction(tr("Create Diagram"), QStringLiteral("createDiagram"), &menu));
        addSeparator = true;
    }
    if (extendContextMenu(&menu))
        addSeparator = true;
    if (addSeparator)
        menu.addSeparator();
    menu.addAction(new ContextMenuAction(tr("Remove"), QStringLiteral("remove"),
                                         QKeySequence(QKeySequence::Delete), &menu));
    menu.addAction(new ContextMenuAction(tr("Delete"), QStringLiteral("delete"),
                                         QKeySequence(Qt::CTRL + Qt::Key_D), &menu));
    //menu.addAction(new ContextMenuAction(tr("Select in Model Tree"), QStringLiteral("selectInModelTree"), &menu));
    QMenu alignMenu;
    alignMenu.setTitle(tr("Align Objects"));
    alignMenu.addAction(new ContextMenuAction(tr("Align Left"), QStringLiteral("alignLeft"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(tr("Center Vertically"), QStringLiteral("centerVertically"),
                                              &alignMenu));
    alignMenu.addAction(new ContextMenuAction(tr("Align Right"), QStringLiteral("alignRight"), &alignMenu));
    alignMenu.addSeparator();
    alignMenu.addAction(new ContextMenuAction(tr("Align Top"), QStringLiteral("alignTop"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(tr("Center Horizontally"), QStringLiteral("centerHorizontally"),
                                              &alignMenu));
    alignMenu.addAction(new ContextMenuAction(tr("Align Bottom"), QStringLiteral("alignBottom"), &alignMenu));
    alignMenu.addSeparator();
    alignMenu.addAction(new ContextMenuAction(tr("Same Width"), QStringLiteral("sameWidth"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(tr("Same Height"), QStringLiteral("sameHeight"), &alignMenu));
    alignMenu.addAction(new ContextMenuAction(tr("Same Size"), QStringLiteral("sameSize"), &alignMenu));
    alignMenu.setEnabled(m_diagramSceneModel->hasMultiObjectsSelection());
    menu.addMenu(&alignMenu);

    QAction *selectedAction = menu.exec(event->screenPos());
    if (selectedAction) {
        if (!handleSelectedContextMenuAction(selectedAction)) {
            auto action = dynamic_cast<ContextMenuAction *>(selectedAction);
            QMT_CHECK(action);
            if (action->id() == QStringLiteral("openDiagram")) {
                m_diagramSceneModel->diagramSceneController()->elementTasks()->openDiagram(m_object, m_diagramSceneModel->diagram());
            } else if (action->id() == QStringLiteral("createDiagram")) {
                m_diagramSceneModel->diagramSceneController()->elementTasks()->createAndOpenDiagram(m_object, m_diagramSceneModel->diagram());
            } else if (action->id() == QStringLiteral("remove")) {
                DSelection selection = m_diagramSceneModel->selectedElements();
                if (selection.isEmpty())
                    selection.append(m_object->uid(), m_diagramSceneModel->diagram()->uid());
                m_diagramSceneModel->diagramController()->deleteElements(selection, m_diagramSceneModel->diagram());
            } else if (action->id() == QStringLiteral("delete")) {
                DSelection selection = m_diagramSceneModel->selectedElements();
                if (selection.isEmpty())
                    selection.append(m_object->uid(), m_diagramSceneModel->diagram()->uid());
                m_diagramSceneModel->diagramSceneController()->deleteFromDiagram(selection, m_diagramSceneModel->diagram());
            } else if (action->id() == QStringLiteral("selectInModelTree")) {
                // TODO implement
            } else if (action->id() == QStringLiteral("alignLeft")) {
                align(IAlignable::AlignLeft, QStringLiteral("left"));
            } else if (action->id() == QStringLiteral("centerVertically")) {
                align(IAlignable::AlignVcenter, QStringLiteral("center"));
            } else if (action->id() == QStringLiteral("alignRight")) {
                align(IAlignable::AlignRight, QStringLiteral("right"));
            } else if (action->id() == QStringLiteral("alignTop")) {
                align(IAlignable::AlignTop, QStringLiteral("top"));
            } else if (action->id() == QStringLiteral("centerHorizontally")) {
                align(IAlignable::AlignHcenter, QStringLiteral("center"));
            } else if (action->id() == QStringLiteral("alignBottom")) {
                align(IAlignable::AlignBottom, QStringLiteral("bottom"));
            } else if (action->id() == QStringLiteral("sameWidth")) {
                align(IAlignable::AlignWidth, QStringLiteral("width"));
            } else if (action->id() == QStringLiteral("sameHeight")) {
                align(IAlignable::AlignHeight, QStringLiteral("height"));
            } else if (action->id() == QStringLiteral("sameSize")) {
                align(IAlignable::AlignSize, QStringLiteral("size"));
            }
        }
    }
}

QSizeF ObjectItem::minimumSize(const QSet<QGraphicsItem *> &items) const
{
    QSizeF minimumSize(0.0, 0.0);
    foreach (QGraphicsItem *item, items) {
        if (auto resizable = dynamic_cast<IResizable *>(item)) {
            QSizeF size = resizable->minimumSize();
            if (size.width() > minimumSize.width())
                minimumSize.setWidth(size.width());
            if (size.height() > minimumSize.height())
                minimumSize.setHeight(size.height());
        }
    }
    return minimumSize;
}

} // namespace qmt
