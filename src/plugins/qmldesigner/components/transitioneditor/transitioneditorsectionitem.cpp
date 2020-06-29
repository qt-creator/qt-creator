/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "transitioneditorsectionitem.h"
#include "transitioneditorgraphicsscene.h"
#include "transitioneditorpropertyitem.h"

#include "timelineactions.h"
#include "timelineconstants.h"
#include "timelineicons.h"
#include "timelinepropertyitem.h"
#include "timelineutils.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <variantproperty.h>
#include <qmltimeline.h>
#include <qmltimelinekeyframegroup.h>

#include <rewritingexception.h>

#include <theme.h>

#include <utils/qtcassert.h>

#include <QAction>
#include <QApplication>
#include <QColorDialog>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

#include <QGraphicsView>

#include <QDebug>

#include <cmath>
#include <limits>

namespace QmlDesigner {

static void scaleDuration(const ModelNode &node, qreal s)
{
    if (node.hasVariantProperty("duration")) {
        qreal old = node.variantProperty("duration").value().toDouble();
        node.variantProperty("duration").setValue(qRound(old * s));
    }
}

static void moveDuration(const ModelNode &node, qreal s)
{
    if (node.hasVariantProperty("duration")) {
        qreal old = node.variantProperty("duration").value().toDouble();
        node.variantProperty("duration").setValue(old + s);
    }
}

class ClickDummy : public TimelineItem
{
public:
    explicit ClickDummy(TransitionEditorSectionItem *parent)
        : TimelineItem(parent)
    {
        setGeometry(0, 0, TimelineConstants::sectionWidth, TimelineConstants::sectionHeight);

        setZValue(10);
        setCursor(Qt::ArrowCursor);
    }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
};

TransitionEditorSectionItem::TransitionEditorSectionItem(TimelineItem *parent)
    : TimelineItem(parent)
{}

TransitionEditorSectionItem *TransitionEditorSectionItem::create(const ModelNode &animation,
                                                                 TimelineItem *parent)
{
    auto item = new TransitionEditorSectionItem(parent);

    ModelNode target;

    if (animation.isValid()) {
        const QList<ModelNode> propertyAnimations = animation.subModelNodesOfType(
            "QtQuick.PropertyAnimation");

        for (const ModelNode &child : propertyAnimations) {
            if (child.hasBindingProperty("target"))
                target = child.bindingProperty("target").resolveToModelNode();
        }
    }

    item->m_targetNode = target;
    item->m_animationNode = animation;
    item->createPropertyItems();

    if (target.isValid())
        item->setToolTip(target.id());

    item->m_dummyItem = new ClickDummy(item);
    item->m_dummyItem->update();

    item->m_barItem = new TransitionEditorBarItem(item);
    item->invalidateBar();
    item->invalidateHeight();

    return item;
}

void TransitionEditorSectionItem::invalidateBar()
{
    qreal min = std::numeric_limits<qreal>::max();
    qreal max = 0;

    if (!m_animationNode.isValid())
        return;

    for (const ModelNode &sequential : m_animationNode.directSubModelNodes()) {
        qreal locMin = 0;
        qreal locMax = 0;

        for (const ModelNode &child : sequential.directSubModelNodes()) {
            if (child.hasMetaInfo() && child.isSubclassOf("QtQuick.PropertyAnimation"))
                locMax = child.variantProperty("duration").value().toDouble();
            else if (child.hasMetaInfo() && child.isSubclassOf("QtQuick.PauseAnimation"))
                locMin = child.variantProperty("duration").value().toDouble();
        }

        locMax = locMax + locMin;

        min = qMin(min, locMin);
        max = qMax(max, locMax);
    }

    const qreal sceneMin = m_barItem->mapFromFrameToScene(min);

    QRectF barRect(sceneMin,
                   0,
                   (max - min) * m_barItem->rulerScaling(),
                   TimelineConstants::sectionHeight - 1);

    m_barItem->setRect(barRect);
}

int TransitionEditorSectionItem::type() const
{
    return Type;
}

void TransitionEditorSectionItem::updateData(QGraphicsItem *item)
{
    if (auto sectionItem = qgraphicsitem_cast<TransitionEditorSectionItem *>(item))
        sectionItem->updateData();
}

void TransitionEditorSectionItem::invalidateBar(QGraphicsItem *item)
{
    if (auto sectionItem = qgraphicsitem_cast<TransitionEditorSectionItem *>(item))
        sectionItem->invalidateBar();
}

void TransitionEditorSectionItem::updateDataForTarget(QGraphicsItem *item,
                                                      const ModelNode &target,
                                                      bool *b)
{
    if (!target.isValid())
        return;

    if (auto sectionItem = qgraphicsitem_cast<TransitionEditorSectionItem *>(item)) {
        if (sectionItem->m_targetNode == target) { //TODO update animation node
            sectionItem->updateData();
            if (b)
                *b = true;
        }
    }
}

void TransitionEditorSectionItem::moveAllDurations(qreal offset)
{
    for (const ModelNode &sequential : m_animationNode.directSubModelNodes()) {
        for (const ModelNode &child : sequential.directSubModelNodes()) {
            if (child.hasMetaInfo() && child.isSubclassOf("QtQuick.PauseAnimation"))
                moveDuration(child, offset);
        }
    }
}

void TransitionEditorSectionItem::scaleAllDurations(qreal scale)
{
    for (const ModelNode &sequential : m_animationNode.directSubModelNodes()) {
        for (const ModelNode &child : sequential.directSubModelNodes()) {
            if (child.hasMetaInfo() && child.isSubclassOf("QtQuick.PropertyAnimation"))
                scaleDuration(child, scale);
        }
    }
}

qreal TransitionEditorSectionItem::firstFrame()
{
    return 0;
    //if (!m_timeline.isValid())
    //return 0;

    //return m_timeline.minActualKeyframe(m_targetNode);
}

AbstractView *TransitionEditorSectionItem::view() const
{
    return m_animationNode.view();
}

bool TransitionEditorSectionItem::isSelected() const
{
    return m_targetNode.isValid() && m_targetNode.isSelected();
}

ModelNode TransitionEditorSectionItem::targetNode() const
{
    return m_targetNode;
}

static QPixmap rotateby90(const QPixmap &pixmap)
{
    QImage sourceImage = pixmap.toImage();
    QImage destImage(pixmap.height(), pixmap.width(), sourceImage.format());

    for (int x = 0; x < pixmap.width(); x++)
        for (int y = 0; y < pixmap.height(); y++)
            destImage.setPixel(y, x, sourceImage.pixel(x, y));

    QPixmap result = QPixmap::fromImage(destImage);

    result.setDevicePixelRatio(pixmap.devicePixelRatio());

    return result;
}

static int devicePixelHeight(const QPixmap &pixmap)
{
    return pixmap.height() / pixmap.devicePixelRatioF();
}

void TransitionEditorSectionItem::paint(QPainter *painter,
                                        const QStyleOptionGraphicsItem * /*option*/,
                                        QWidget *)
{
    if (m_targetNode.isValid()) {
        painter->save();

        const QColor textColor = Theme::getColor(Theme::PanelTextColorLight);
        const QColor penColor = Theme::instance()->qmlDesignerBackgroundColorDarker();
        QColor brushColor = Theme::getColor(Theme::BackgroundColorDark);

        int fillOffset = 0;
        if (isSelected()) {
            brushColor = Theme::getColor(Theme::QmlDesigner_HighlightColor);
            fillOffset = 1;
        }

        painter->fillRect(0,
                          0,
                          TimelineConstants::sectionWidth,
                          TimelineConstants::sectionHeight - fillOffset,
                          brushColor);
        painter->fillRect(TimelineConstants::sectionWidth,
                          0,
                          size().width() - TimelineConstants::sectionWidth,
                          size().height(),
                          Theme::instance()->qmlDesignerBackgroundColorDarkAlternate());

        painter->setPen(penColor);
        drawLine(painter,
                 TimelineConstants::sectionWidth - 1,
                 0,
                 TimelineConstants::sectionWidth - 1,
                 size().height() - 1);
        drawLine(painter,
                 TimelineConstants::sectionWidth,
                 TimelineConstants::sectionHeight - 1,
                 size().width(),
                 TimelineConstants::sectionHeight - 1);

        static const QPixmap arrow = Theme::getPixmap("down-arrow");

        static const QPixmap arrow90 = rotateby90(arrow);

        const QPixmap rotatedArrow = collapsed() ? arrow90 : arrow;

        const int textOffset = QFontMetrics(font()).ascent()
                               + (TimelineConstants::sectionHeight - QFontMetrics(font()).height())
                                     / 2;

        painter->drawPixmap(collapsed() ? 6 : 4,
                            (TimelineConstants::sectionHeight - devicePixelHeight(rotatedArrow)) / 2,
                            rotatedArrow);

        painter->setPen(textColor);

        QFontMetrics fm(painter->font());
        const QString elidedId = fm.elidedText(m_targetNode.id(),
                                               Qt::ElideMiddle,
                                               TimelineConstants::sectionWidth
                                                   - TimelineConstants::textIndentationSections);
        painter->drawText(TimelineConstants::textIndentationSections, textOffset, elidedId);

        painter->restore();
    }
}

void TransitionEditorSectionItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->pos().y() > TimelineConstants::sectionHeight
        || event->pos().x() < TimelineConstants::textIndentationSections) {
        TimelineItem::mouseDoubleClickEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        event->accept();
        toggleCollapsed();
    }
}

void TransitionEditorSectionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->pos().y() > TimelineConstants::sectionHeight) {
        TimelineItem::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton)
        event->accept();
}

void TransitionEditorSectionItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->pos().y() > TimelineConstants::sectionHeight) {
        TimelineItem::mouseReleaseEvent(event);
        return;
    }

    if (event->button() != Qt::LeftButton)
        return;

    event->accept();

    if (event->pos().x() > TimelineConstants::textIndentationSections
        && event->button() == Qt::LeftButton) {
        if (m_targetNode.isValid())
            m_targetNode.view()->setSelectedModelNode(m_targetNode);
    } else {
        toggleCollapsed();
    }
    update();
}

void TransitionEditorSectionItem::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    TimelineItem::resizeEvent(event);

    for (auto child : propertyItems()) {
        TransitionEditorPropertyItem *item = static_cast<TransitionEditorPropertyItem *>(child);
        item->resize(size().width(), TimelineConstants::sectionHeight);
    }
}

void TransitionEditorSectionItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *) {}

void TransitionEditorSectionItem::updateData()
{
    invalidateBar();
    resize(rulerWidth(), size().height());
    invalidateProperties();
    update();
}

const QList<QGraphicsItem *> TransitionEditorSectionItem::propertyItems() const
{
    QList<QGraphicsItem *> list;

    const QList<QGraphicsItem *> children = childItems();
    for (auto child : children) {
        if (m_barItem != child && m_dummyItem != child)
            list.append(child);
    }

    return list;
}

void TransitionEditorSectionItem::invalidateHeight()
{
    int height = 0;
    bool visible = true;

    if (collapsed()) {
        height = TimelineConstants::sectionHeight;
        visible = false;
    } else {
        const QList<ModelNode> propertyAnimations = m_animationNode.subModelNodesOfType(
            "QtQuick.PropertyAnimation");

        height = TimelineConstants::sectionHeight
                 + propertyAnimations.count() * TimelineConstants::sectionHeight;
        visible = true;
    }

    for (auto child : propertyItems())
        child->setVisible(visible);

    setPreferredHeight(height);
    setMinimumHeight(height);
    setMaximumHeight(height);

    auto transitionScene = qobject_cast<TransitionEditorGraphicsScene *>(scene());
    transitionScene->activateLayout();
}

void TransitionEditorSectionItem::createPropertyItems()
{
    int yPos = TimelineConstants::sectionHeight;
    const QList<ModelNode> propertyAnimations = m_animationNode.subModelNodesOfType(
        "QtQuick.PropertyAnimation");
    for (const auto &anim : propertyAnimations) {
        auto item = TransitionEditorPropertyItem::create(anim, this);
        item->setY(yPos);
        yPos = yPos + TimelineConstants::sectionHeight;
    }
}

void TransitionEditorSectionItem::invalidateProperties()
{
    for (auto child : propertyItems()) {
        delete child;
    }

    createPropertyItems();

    for (auto child : propertyItems()) {
        TransitionEditorPropertyItem *item = static_cast<TransitionEditorPropertyItem *>(child);
        item->updateData();
        item->resize(size().width(), TimelineConstants::sectionHeight);
    }
    invalidateHeight();
}

bool TransitionEditorSectionItem::collapsed() const
{
    return m_targetNode.isValid() && !m_targetNode.hasAuxiliaryData("timeline_expanded");
}

qreal TransitionEditorSectionItem::rulerWidth() const
{
    return static_cast<TimelineGraphicsScene *>(scene())->rulerWidth();
}

void TransitionEditorSectionItem::toggleCollapsed()
{
    QTC_ASSERT(m_targetNode.isValid(), return );

    if (collapsed())
        m_targetNode.setAuxiliaryData("timeline_expanded", true);
    else
        m_targetNode.removeAuxiliaryData("timeline_expanded");

    invalidateHeight();
}

TransitionEditorBarItem::TransitionEditorBarItem(TransitionEditorSectionItem *parent)
    : TimelineMovableAbstractItem(parent)
{
    setAcceptHoverEvents(true);
    setPen(Qt::NoPen);
}

TransitionEditorBarItem::TransitionEditorBarItem(TransitionEditorPropertyItem *parent)
    : TimelineMovableAbstractItem(parent)
{
    setAcceptHoverEvents(true);
    setPen(Qt::NoPen);
}

void TransitionEditorBarItem::itemMoved(const QPointF &start, const QPointF &end)
{
    if (isActiveHandle(Location::Undefined))
        dragInit(rect(), start);

    qreal min = qreal(TimelineConstants::sectionWidth + TimelineConstants::timelineLeftOffset
                      - scrollOffset());
    qreal max = qreal(abstractScrollGraphicsScene()->rulerWidth() - TimelineConstants::sectionWidth
                      + rect().width());

    const qreal minFrameX = mapFromFrameToScene(abstractScrollGraphicsScene()->startFrame());
    const qreal maxFrameX = mapFromFrameToScene(abstractScrollGraphicsScene()->endFrame());

    if (min < minFrameX)
        min = minFrameX;

    if (max > maxFrameX)
        max = maxFrameX;

    if (isActiveHandle(Location::Center))
        dragCenter(rect(), end, min, max);
    else
        dragHandle(rect(), end, min, max);

    emit abstractScrollGraphicsScene()->statusBarMessageChanged(
        tr("Range from %1 to %2")
            .arg(qRound(mapFromSceneToFrame(rect().x())))
            .arg(qRound(mapFromSceneToFrame(rect().width() + rect().x()))));
}

void TransitionEditorBarItem::commitPosition(const QPointF & /*point*/)
{
    if (sectionItem() && sectionItem()->view()) {
        if (m_handle != Location::Undefined) {
            sectionItem()
                ->view()
                ->executeInTransaction("TransitionEditorBarItem::commitPosition", [this]() {
                    qreal scaleFactor = rect().width() / m_oldRect.width();

                    qreal moved = (rect().topLeft().x() - m_oldRect.topLeft().x()) / rulerScaling();
                    qreal supposedFirstFrame = qRound(moved);

                    sectionItem()->scaleAllDurations(scaleFactor);
                    sectionItem()->moveAllDurations(supposedFirstFrame);
                    sectionItem()->updateData();
                });
        }
    } else if (propertyItem() && propertyItem()->view()) {
        if (m_handle != Location::Undefined) {
            propertyItem()
                ->view()
                ->executeInTransaction("TransitionEditorBarItem::commitPosition", [this]() {
                    qreal scaleFactor = rect().width() / m_oldRect.width();
                    qreal moved = (rect().topLeft().x() - m_oldRect.topLeft().x()) / rulerScaling();
                    qreal supposedFirstFrame = qRound(moved);
                    scaleDuration(propertyItem()->propertyAnimation(), scaleFactor);
                    moveDuration(propertyItem()->pauseAnimation(), supposedFirstFrame);
                    propertyItem()->updateData();
                    propertyItem()->updateParentData();
                });
        }
    }

    m_handle = Location::Undefined;
    m_bounds = Location::Undefined;
    m_pivot = 0.0;
    m_oldRect = QRectF();
    scrollOffsetChanged();
}

void TransitionEditorBarItem::scrollOffsetChanged()
{
    if (sectionItem())
        sectionItem()->invalidateBar();
    else if (propertyItem())
        propertyItem()->invalidateBar();
}

void TransitionEditorBarItem::paint(QPainter *painter,
                                    const QStyleOptionGraphicsItem *option,
                                    QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QColor brushColor = Theme::getColor(Theme::QmlDesigner_HighlightColor);
    QColor brushColorSection = Theme::getColor(Theme::QmlDesigner_HighlightColor).darker(120);
    QColor penColor = Theme::getColor(Theme::QmlDesigner_HighlightColor).lighter(140);

    const QRectF itemRect = rect();

    painter->save();
    painter->setClipRect(TimelineConstants::sectionWidth,
                         0,
                         itemRect.width() + itemRect.x(),
                         itemRect.height());

    if (sectionItem())
        painter->fillRect(itemRect, brushColorSection);
    else
        painter->fillRect(itemRect, brushColor);

    if (propertyItem() && propertyItem()->isSelected()) {
        painter->setPen(penColor);
        painter->drawRect(itemRect.adjusted(0, 0, 0, -1));
    }

    painter->restore();
}

void TransitionEditorBarItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    const auto p = event->pos();

    QRectF left, right;
    if (handleRects(rect(), left, right)) {
        if (left.contains(p) || right.contains(p)) {
            if (cursor().shape() != Qt::SizeHorCursor)
                setCursor(QCursor(Qt::SizeHorCursor));
        } else if (rect().contains(p)) {
            if (cursor().shape() != Qt::ClosedHandCursor)
                setCursor(QCursor(Qt::ClosedHandCursor));
        }
    } else {
        if (rect().contains(p))
            setCursor(QCursor(Qt::ClosedHandCursor));
    }
}

void TransitionEditorBarItem::contextMenuEvent(QGraphicsSceneContextMenuEvent * /*event*/) {}

void TransitionEditorBarItem::mousePressEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    if (propertyItem())
        propertyItem()->select();
}

TransitionEditorSectionItem *TransitionEditorBarItem::sectionItem() const
{
    return qgraphicsitem_cast<TransitionEditorSectionItem *>(parentItem());
}

TransitionEditorPropertyItem *TransitionEditorBarItem::propertyItem() const
{
    return qgraphicsitem_cast<TransitionEditorPropertyItem *>(parentItem());
}

void TransitionEditorBarItem::dragInit(const QRectF &rect, const QPointF &pos)
{
    QRectF left, right;
    m_oldRect = rect;
    if (handleRects(rect, left, right)) {
        if (left.contains(pos)) {
            m_handle = Location::Left;
            m_pivot = pos.x() - left.topLeft().x();
        } else if (right.contains(pos)) {
            m_handle = Location::Right;
            m_pivot = pos.x() - right.topRight().x();
        } else if (rect.contains(pos)) {
            m_handle = Location::Center;
            m_pivot = pos.x() - rect.topLeft().x();
        }

    } else {
        if (rect.contains(pos)) {
            m_handle = Location::Center;
            m_pivot = pos.x() - rect.topLeft().x();
        }
    }
}

void TransitionEditorBarItem::dragCenter(QRectF rect, const QPointF &pos, qreal min, qreal max)
{
    if (validateBounds(pos.x() - rect.topLeft().x())) {
        qreal targetX = pos.x() - m_pivot;

        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // snapping
            qreal snappedTargetFrame = abstractScrollGraphicsScene()->snap(mapFromSceneToFrame(targetX));
            targetX = mapFromFrameToScene(snappedTargetFrame);
        }
        rect.moveLeft(targetX);
        if (rect.topLeft().x() < min) {
            rect.moveLeft(min);
            setOutOfBounds(Location::Left);
        } else if (rect.topRight().x() > max) {
            rect.moveRight(max);
            setOutOfBounds(Location::Right);
        }
        setRect(rect);
    }
}

void TransitionEditorBarItem::dragHandle(QRectF rect, const QPointF &pos, qreal min, qreal max)
{
    QRectF left, right;
    handleRects(rect, left, right);

    if (isActiveHandle(Location::Left)) {
        if (validateBounds(pos.x() - left.topLeft().x())) {
            qreal targetX = pos.x() - m_pivot;
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // snapping
                qreal snappedTargetFrame = abstractScrollGraphicsScene()->snap(mapFromSceneToFrame(targetX));
                targetX = mapFromFrameToScene(snappedTargetFrame);
            }
            rect.setLeft(targetX);
            if (rect.left() < min) {
                rect.setLeft(min);
                setOutOfBounds(Location::Left);
            } else if (rect.left() >= rect.right() - minimumBarWidth)
                rect.setLeft(rect.right() - minimumBarWidth);

            setRect(rect);
        }
    } else if (isActiveHandle(Location::Right)) {
        if (validateBounds(pos.x() - right.topRight().x())) {
            qreal targetX = pos.x() - m_pivot;
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // snapping
                qreal snappedTargetFrame = abstractScrollGraphicsScene()->snap(mapFromSceneToFrame(targetX));
                targetX = mapFromFrameToScene(snappedTargetFrame);
            }
            rect.setRight(targetX);
            if (rect.right() > max) {
                rect.setRight(max);
                setOutOfBounds(Location::Right);
            } else if (rect.right() <= rect.left() + minimumBarWidth)
                rect.setRight(rect.left() + minimumBarWidth);

            setRect(rect);
        }
    }
}

bool TransitionEditorBarItem::handleRects(const QRectF &rect, QRectF &left, QRectF &right) const
{
    if (rect.width() < minimumBarWidth)
        return false;

    const qreal handleSize = rect.height();

    auto handleRect = QRectF(0, 0, handleSize, handleSize);
    handleRect.moveCenter(rect.center());

    handleRect.moveLeft(rect.left());
    left = handleRect;

    handleRect.moveRight(rect.right());
    right = handleRect;

    return true;
}

bool TransitionEditorBarItem::isActiveHandle(Location location) const
{
    return m_handle == location;
}

void TransitionEditorBarItem::setOutOfBounds(Location location)
{
    m_bounds = location;
    update();
}

bool TransitionEditorBarItem::validateBounds(qreal distance)
{
    update();
    if (m_bounds == Location::Left) {
        if (distance > m_pivot)
            m_bounds = Location::Center;
        return false;

    } else if (m_bounds == Location::Right) {
        if (distance < m_pivot)
            m_bounds = Location::Center;
        return false;
    }
    return true;
}

} // namespace QmlDesigner
