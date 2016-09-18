/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "connectableitem.h"
#include "cornergrabberitem.h"
#include "graphicsscene.h"
#include "highlightitem.h"
#include "quicktransitionitem.h"
#include "sceneutils.h"
#include "scxmleditorconstants.h"
#include "serializer.h"
#include "stateitem.h"

#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QStringList>
#include <QUndoStack>

using namespace ScxmlEditor::PluginInterface;

ConnectableItem::ConnectableItem(const QPointF &p, BaseItem *parent)
    : BaseItem(parent)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setAcceptDrops(true);

    m_selectedPen.setStyle(Qt::DotLine);
    m_selectedPen.setColor(QColor(0x44, 0x44, 0xed));
    m_selectedPen.setCosmetic(true);
    m_releasedFromParentBrush = QBrush(QColor(0x98, 0x98, 0x98));

    setPos(p);
    connect(this, &ConnectableItem::geometryChanged, this, &ConnectableItem::updateCornerPositions);
}

ConnectableItem::~ConnectableItem()
{
    setBlockUpdates(true);

    foreach (ConnectableItem *item, m_overlappedItems) {
        item->removeOverlappingItem(this);
    }
    m_overlappedItems.clear();

    foreach (TransitionItem *transition, m_outputTransitions) {
        transition->disconnectItem(this);
    }
    m_outputTransitions.clear();

    foreach (TransitionItem *transition, m_inputTransitions) {
        transition->disconnectItem(this);
    }
    m_inputTransitions.clear();

    qDeleteAll(m_quickTransitions);
    m_quickTransitions.clear();
}

void ConnectableItem::createCorners()
{
    for (int i = 0; i < 8; ++i) {
        Qt::CursorShape cur = Qt::SizeHorCursor;
        switch (i) {
        case 0:
        case 4:
            cur = Qt::SizeFDiagCursor;
            break;
        case 1:
        case 5:
            cur = Qt::SizeVerCursor;
            break;
        case 2:
        case 6:
            cur = Qt::SizeBDiagCursor;
            break;
        case 3:
        case 7:
            cur = Qt::SizeHorCursor;
            break;
        default:
            break;
        }
        m_corners << new CornerGrabberItem(this, cur);
    }

    qDeleteAll(m_quickTransitions);
    m_quickTransitions.clear();
    m_quickTransitions << new QuickTransitionItem(0, UnknownType, this);
    m_quickTransitions << new QuickTransitionItem(1, StateType, this);
    m_quickTransitions << new QuickTransitionItem(2, ParallelType, this);
    m_quickTransitions << new QuickTransitionItem(3, HistoryType, this);
    m_quickTransitions << new QuickTransitionItem(4, FinalStateType, this);

    updateCornerPositions();
}

void ConnectableItem::removeCorners()
{
    qDeleteAll(m_corners);
    m_corners.clear();

    qDeleteAll(m_quickTransitions);
    m_quickTransitions.clear();
}

void ConnectableItem::updateCornerPositions()
{
    QRectF r = boundingRect();
    if (m_corners.count() == 8) {
        qreal cx = r.center().x();
        qreal cy = r.center().y();
        m_corners[0]->setPos(r.topLeft());
        m_corners[1]->setPos(cx, r.top());
        m_corners[2]->setPos(r.topRight());
        m_corners[3]->setPos(r.right(), cy);
        m_corners[4]->setPos(r.bottomRight());
        m_corners[5]->setPos(cx, r.bottom());
        m_corners[6]->setPos(r.bottomLeft());
        m_corners[7]->setPos(r.left(), cy);
    }

    for (int i = 0; i < m_quickTransitions.count(); ++i) {
        m_quickTransitions[i]->setPos(r.topLeft());
        m_quickTransitions[i]->setVisible(!m_releasedFromParent && canStartTransition(m_quickTransitions[i]->connectionType()));
    }
    updateShadowClipRegion();
}

bool ConnectableItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (watched->type() == CornerGrabberType) {
        auto c = qgraphicsitem_cast<CornerGrabberItem*>(watched);
        auto mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
        if (!c || !mouseEvent)
            return BaseItem::sceneEventFilter(watched, event);

        QRectF r = boundingRect();
        if (event->type() == QEvent::GraphicsSceneMousePress) {
            setOpacity(0.5);
        } else if (event->type() == QEvent::GraphicsSceneMouseMove) {
            QPointF pw = watched->mapToItem(this, mouseEvent->pos());
            QRectF rMin;
            if (type() >= StateType)
                rMin = qgraphicsitem_cast<StateItem*>(this)->childItemsBoundingRect();
            if (rMin.isNull()) {
                rMin = QRectF(0, 0, m_minimumWidth, m_minimumHeight);
                rMin.moveCenter(r.center());
            }

            switch (m_corners.indexOf(c)) {
            case 0:
                r.setTopLeft(QPointF(qMin(pw.x(), rMin.left()), qMin(pw.y(), rMin.top())));
                break;
            case 1:
                r.setTop(qMin(pw.y(), rMin.top()));
                break;
            case 2:
                r.setTopRight(QPointF(qMax(pw.x(), rMin.right()), qMin(pw.y(), rMin.top())));
                break;
            case 3:
                r.setRight(qMax(pw.x(), rMin.right()));
                break;
            case 4:
                r.setBottomRight(QPointF(qMax(pw.x(), rMin.right()), qMax(pw.y(), rMin.bottom())));
                break;
            case 5:
                r.setBottom(qMax(pw.y(), rMin.bottom()));
                break;
            case 6:
                r.setBottomLeft(QPointF(qMin(pw.x(), rMin.left()), qMax(pw.y(), rMin.bottom())));
                break;
            case 7:
                r.setLeft(qMin(pw.x(), rMin.left()));
                break;
            default:
                break;
            }

            setItemBoundingRect(r);
            updateCornerPositions();
            updateTransitions();

            return true;
        } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            setOpacity(1.0);
            updateUIProperties();
            checkOverlapping();
        }
    } else if (watched->type() == QuickTransitionType) {
        auto mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
        if (!mouseEvent)
            return BaseItem::sceneEventFilter(watched, event);

        if (event->type() == QEvent::GraphicsSceneMousePress) {
            m_newTransitionStartedPoint = mouseEvent->pos();
            tag()->document()->undoStack()->beginMacro(tr("Add new state"));

            m_newTransition = new TransitionItem;
            scene()->addItem(m_newTransition);

            ScxmlTag *newTag = nullptr;
            if (tag()->tagType() == Initial || tag()->tagType() == History)
                newTag = new ScxmlTag(InitialTransition, tag()->document());
            else
                newTag = new ScxmlTag(Transition, tag()->document());
            newTag->setAttribute("type", "external");
            m_newTransition->init(newTag);

            tag()->document()->addTag(tag(), newTag);
            m_newTransition->startTransitionFrom(this, watched->mapToScene(mouseEvent->pos()));
            return true;
        } else if (event->type() == QEvent::GraphicsSceneMouseMove) {
            if (m_newTransition)
                m_newTransition->setEndPos(watched->mapToScene(mouseEvent->pos()));
        } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            auto tra = qgraphicsitem_cast<QuickTransitionItem*>(watched);
            if (!tra)
                return BaseItem::sceneEventFilter(watched, event);

            if (m_newTransition) {
                QLineF line(m_newTransitionStartedPoint, mouseEvent->pos());
                if (line.length() > 30) {
                    m_newTransition->connectToTopItem(watched->mapToScene(mouseEvent->pos()), TransitionItem::End, tra->connectionType());
                    m_newTransition = nullptr;
                    setSelected(false);
                    tag()->document()->undoStack()->endMacro();
                } else {
                    m_newTransition->grabMouse(tra->connectionType());
                    m_newTransition = nullptr;
                    setSelected(false);
                }
                return true;
            }
        }
    }

    return BaseItem::sceneEventFilter(watched, event);
}

void ConnectableItem::addOutputTransition(TransitionItem *transition)
{
    m_outputTransitions.append(transition);
    transitionCountChanged();
}

void ConnectableItem::addInputTransition(TransitionItem *transition)
{
    m_inputTransitions.append(transition);
    transitionCountChanged();
}

void ConnectableItem::removeOutputTransition(TransitionItem *transition)
{
    m_outputTransitions.removeAll(transition);
    transitionCountChanged();
}

void ConnectableItem::removeInputTransition(TransitionItem *transition)
{
    m_inputTransitions.removeAll(transition);
    transitionCountChanged();
}

void ConnectableItem::updateInputTransitions()
{
    foreach (TransitionItem *transition, m_inputTransitions) {
        transition->updateComponents();
        transition->updateUIProperties();
    }
    transitionsChanged();
}

void ConnectableItem::updateOutputTransitions()
{
    foreach (TransitionItem *transition, m_outputTransitions) {
        transition->updateComponents();
        transition->updateUIProperties();
    }
    transitionsChanged();
}

void ConnectableItem::updateTransitions(bool allChildren)
{
    updateOutputTransitions();
    updateInputTransitions();

    if (allChildren) {
        foreach (QGraphicsItem *it, childItems()) {
            auto item = static_cast<ConnectableItem*>(it);
            if (item && item->type() >= InitialStateType)
                item->updateTransitions(allChildren);
        }
    }
}

void ConnectableItem::updateTransitionAttributes(bool allChildren)
{
    foreach (TransitionItem *transition, m_outputTransitions)
        transition->updateTarget();

    foreach (TransitionItem *transition, m_inputTransitions)
        transition->updateTarget();

    if (allChildren) {
        foreach (QGraphicsItem *it, childItems()) {
            auto item = static_cast<ConnectableItem*>(it);
            if (item && item->type() >= InitialStateType)
                item->updateTransitionAttributes(allChildren);
        }
    }
}

void ConnectableItem::transitionsChanged()
{
}

void ConnectableItem::transitionCountChanged()
{
}

QPointF ConnectableItem::getInternalPosition(const TransitionItem *transition, TransitionItem::TransitionTargetType type) const
{
    const QRectF srect = sceneBoundingRect();

    int ind = 0;
    if (type == TransitionItem::InternalNoTarget) {
        foreach (TransitionItem *item, m_outputTransitions) {
            if (item->targetType() == TransitionItem::InternalSameTarget)
                ind++;
        }
    }

    for (int i = 0; i < m_outputTransitions.count(); ++i) {
        TransitionItem *item = m_outputTransitions[i];
        if (item == transition)
            break;
        else if (item->targetType() == type)
            ind++;
    }

    return QPointF(srect.left() + 20, srect.top() + srect.height() * 0.06 + 40 + ind * 30);
}

void ConnectableItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // We want to pan scene when Shift is pressed -> that's why ignore mouse-event here
    if (event->modifiers() & Qt::ShiftModifier) {
        event->ignore();
        return;
    }

    BaseItem::mousePressEvent(event);
}

void ConnectableItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // We want to pan scene when Shift is pressed -> that's why ignore mouse-event here
    if (event->modifiers() & Qt::ShiftModifier) {
        event->ignore();
        return;
    }

    if (!m_moveMacroStarted) {
        m_moveMacroStarted = true;
        tag()->document()->undoStack()->beginMacro(tr("Move State"));
    }

    //Restore old behavior if ctrl & alt modifiers are present
    if (!m_releasedFromParent && !(event->modifiers() & Qt::AltModifier) && !(event->modifiers() & Qt::ControlModifier)) {
        releaseFromParent();
        foreach (QGraphicsItem *it, scene()->selectedItems()) {
            if (it->type() >= InitialStateType && it != this) {
                qgraphicsitem_cast<ConnectableItem*>(it)->releaseFromParent();
            }
        }
    } else
        setOpacity(0.5);

    BaseItem::mouseMoveEvent(event);
}

void ConnectableItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // We want to pan scene when Shift is pressed -> that's why ignore mouse-event here
    if (event->modifiers() & Qt::ShiftModifier) {
        event->ignore();
        return;
    }

    BaseItem::mouseReleaseEvent(event);

    if (m_releasedFromParent) {
        // Try to find parent
        QList<QGraphicsItem*> parentItems = scene()->items(event->scenePos());
        BaseItem *parentItem = nullptr;
        for (int i = 0; i < parentItems.count(); ++i) {
            auto item = static_cast<BaseItem*>(parentItems[i]);
            if (item && item != this && !item->isSelected()
                && item->type() >= StateType
                && SceneUtils::canDrop(item->type(), type())) {
                parentItem = item;
                break;
            }
        }
        connectToParent(parentItem);
        foreach (QGraphicsItem *it, scene()->selectedItems()) {
            if (it->type() >= InitialStateType && it != this)
                qgraphicsitem_cast<ConnectableItem*>(it)->connectToParent(parentItem);
        }
    } else
        setOpacity(1.0);

    if (m_moveMacroStarted) {
        m_moveMacroStarted = false;
        tag()->document()->undoStack()->endMacro();
    }

    checkOverlapping();
}

void ConnectableItem::releaseFromParent()
{
    m_releasedFromParent = true;
    setOpacity(0.5);
    m_releasedIndex = tag()->index();
    m_releasedParent = parentItem();
    tag()->document()->changeParent(tag(), 0, !m_releasedParent ? m_releasedIndex : -1);
    setZValue(503);

    for (int i = 0; i < m_quickTransitions.count(); ++i)
        m_quickTransitions[i]->setVisible(false);
    for (int i = 0; i < m_corners.count(); ++i)
        m_corners[i]->setVisible(false);
    update();
}

void ConnectableItem::connectToParent(BaseItem *parentItem)
{
    for (int i = 0; i < m_quickTransitions.count(); ++i)
        m_quickTransitions[i]->setVisible(canStartTransition(m_quickTransitions[i]->connectionType()));
    for (int i = 0; i < m_corners.count(); ++i)
        m_corners[i]->setVisible(true);

    tag()->document()->changeParent(tag(), parentItem ? parentItem->tag() : 0, parentItem == m_releasedParent ? m_releasedIndex : -1);

    setZValue(0);
    m_releasedIndex = -1;
    m_releasedParent = nullptr;
    m_releasedFromParent = false;
    setOpacity(1.0);
}

QVariant ConnectableItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemSelectedHasChanged: {
        if (value.toBool()) {
            createCorners();
            SceneUtils::moveTop(this, static_cast<GraphicsScene*>(scene()));
        } else
            removeCorners();
        break;
    }
    case ItemParentHasChanged:
        updateTransitions(true);
        updateTransitionAttributes(true);
        // FIXME: intended fallthrough?
    case ItemPositionHasChanged:
        if (!m_releasedFromParent && !blockUpdates())
            checkParentBoundingRect();
        break;
    case ItemScenePositionHasChanged:
        updateTransitions();
        if (m_highlighItem)
            m_highlighItem->advance(1);
        break;
    default:
        break;
    }

    return BaseItem::itemChange(change, value);
}

qreal ConnectableItem::getOpacity()
{
    if (opacity() < 1.0)
        return opacity();

    if (overlapping())
        return 0.5;

    if (parentBaseItem())
        if (parentBaseItem()->type() >= InitialStateType)
            return static_cast<ConnectableItem*>(parentBaseItem())->getOpacity();

    return 1;
}

void ConnectableItem::updateShadowClipRegion()
{
    QPainterPath br, sr;
    //StateItem Background rounded rectangle
    br.addRoundedRect(boundingRect().adjusted(5, 5, -5, -5), 10, 10);
    //Shadow rounded rectangle
    sr.addRoundedRect(boundingRect().adjusted(10, 10, 0, 0), 10, 10);
    //Clippath is subtract
    m_shadowClipPath = sr - br;
}

void ConnectableItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(getOpacity());

    if (m_releasedFromParent) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_releasedFromParentBrush);
        painter->setClipping(true);
        painter->setClipPath(m_shadowClipPath);
        //Since the form is already cliped just draw a rectangle
        painter->drawRect(boundingRect().adjusted(10, 10, 0, 0));
        painter->setClipping(false);
    }

    if (isSelected()) {
        painter->setPen(m_selectedPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }

    painter->restore();
}

void ConnectableItem::updateUIProperties()
{
    if (tag() != 0 && isActiveScene()) {
        Serializer s;
        s.append(pos());
        s.append(boundingRect());
        setEditorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY, s.data());
        s.clear();
        s.append(scenePos());
        s.append(sceneBoundingRect());
        setEditorInfo("scenegeometry", s.data());
    }
}

void ConnectableItem::updateAttributes()
{
    BaseItem::updateAttributes();

    foreach (TransitionItem *transition, m_inputTransitions) {
        if (transition->isEndItem(this))
            transition->setTagValue("target", itemId());
    }
    updateInputTransitions();

    update();
}

void ConnectableItem::updateEditorInfo(bool allChildren)
{
    BaseItem::updateEditorInfo(allChildren);
    updateTransitions();
}

void ConnectableItem::moveStateBy(qreal dx, qreal dy)
{
    moveBy(dx, dy);
    updateUIProperties();
    updateTransitions();
}

void ConnectableItem::setHighlight(bool hl)
{
    BaseItem::setHighlight(hl);
    if (highlight()) {
        if (!m_highlighItem) {
            m_highlighItem = new HighlightItem(this);
            scene()->addItem(m_highlighItem);
        }
    } else {
        delete m_highlighItem;
        m_highlighItem = nullptr;
    }

    if (m_highlighItem)
        m_highlighItem->advance(0);
}

void ConnectableItem::readUISpecifiedProperties(const ScxmlTag *tag)
{
    if (tag) {
        QString data = editorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY);
        if (!data.isEmpty()) {
            QPointF p(0, 0);
            QRectF r(-60, 50, 120, 100);

            Serializer s;
            s.setData(data);
            s.read(p);
            s.read(r);

            setItemBoundingRect(r);
            setPos(p);
        }
    }
}

void ConnectableItem::addTransitions(const ScxmlTag *tag)
{
    if (scene()) {
        for (int i = 0; i < tag->childCount(); ++i) {
            ScxmlTag *child = tag->child(i);
            if (child->tagType() == Transition || child->tagType() == InitialTransition) {
                auto transition = new TransitionItem;
                scene()->addItem(transition);
                transition->setStartItem(this);
                transition->init(child);
            }
        }
    }
}

void ConnectableItem::init(ScxmlTag *tag, BaseItem *parentItem, bool initChildren, bool /*blockUpdates*/)
{
    BaseItem::init(tag, parentItem);
    if (initChildren)
        addTransitions(tag);
}

void ConnectableItem::setMinimumWidth(int width)
{
    m_minimumWidth = width;
    QRectF r = boundingRect();
    if (r.width() < width) {
        r.setWidth(width);
        setItemBoundingRect(r);
    }
}

void ConnectableItem::setMinimumHeight(int height)
{
    m_minimumHeight = height;
    QRectF r = boundingRect();
    if (r.height() < height) {
        r.setHeight(height);
        setItemBoundingRect(r);
    }
}

void ConnectableItem::finalizeCreation()
{
    bool old = blockUpdates();
    setBlockUpdates(true);

    updateAttributes();
    updateEditorInfo();
    updateUIProperties();
    checkInitial(true);

    if (!old)
        setBlockUpdates(false);
}

bool ConnectableItem::hasInputTransitions(const ConnectableItem *parentItem, bool checkChildren) const
{
    foreach (const TransitionItem *it, m_inputTransitions) {
        if (!SceneUtils::isChild(parentItem, it->connectedItem(this)))
            return true;
    }

    if (checkChildren) {
        foreach (QGraphicsItem *it, childItems()) {
            if (it->type() >= InitialStateType) {
                auto item = qgraphicsitem_cast<ConnectableItem*>(it);
                if (item && item->hasInputTransitions(parentItem, checkChildren))
                    return true;
            }
        }
    }

    return false;
}

bool ConnectableItem::hasOutputTransitions(const ConnectableItem *parentItem, bool checkChildren) const
{
    foreach (TransitionItem *it, m_outputTransitions) {
        if (!SceneUtils::isChild(parentItem, it->connectedItem(this)))
            return true;
    }

    if (checkChildren) {
        foreach (QGraphicsItem *it, childItems()) {
            if (it->type() >= InitialStateType) {
                auto item = qgraphicsitem_cast<ConnectableItem*>(it);
                if (item && item->hasOutputTransitions(parentItem, checkChildren))
                    return true;
            }
        }
    }

    return false;
}

void ConnectableItem::addOverlappingItem(ConnectableItem *item)
{
    if (!m_overlappedItems.contains(item))
        m_overlappedItems.append(item);

    setOverlapping(m_overlappedItems.count() > 0);
}

void ConnectableItem::removeOverlappingItem(ConnectableItem *item)
{
    if (m_overlappedItems.contains(item))
        m_overlappedItems.removeAll(item);

    setOverlapping(m_overlappedItems.count() > 0);
}

void ConnectableItem::checkOverlapping()
{
    QVector<ConnectableItem*> overlappedItems;
    foreach (QGraphicsItem *it, collidingItems()) {
        if (it->type() >= InitialStateType && it->parentItem() == parentItem()) {
            overlappedItems << qgraphicsitem_cast<ConnectableItem*>(it);
        }
    }

    // Remove unnecessary items
    for (int i = m_overlappedItems.count(); i--;) {
        if (!overlappedItems.contains(m_overlappedItems[i])) {
            m_overlappedItems[i]->removeOverlappingItem(this);
            m_overlappedItems.removeAt(i);
        }
    }

    // Add new overlapped items
    foreach (ConnectableItem *it, overlappedItems) {
        if (!m_overlappedItems.contains(it)) {
            m_overlappedItems << it;
            it->addOverlappingItem(this);
        }
    }

    setOverlapping(m_overlappedItems.count() > 0);
}

bool ConnectableItem::canStartTransition(ItemType type) const
{
    Q_UNUSED(type)
    return true;
}

QVector<TransitionItem*> ConnectableItem::outputTransitions() const
{
    return m_outputTransitions;
}

QVector<TransitionItem*> ConnectableItem::inputTransitions() const
{
    return m_inputTransitions;
}

int ConnectableItem::transitionCount() const
{
    return m_outputTransitions.count() + m_inputTransitions.count();
}

int ConnectableItem::outputTransitionCount() const
{
    return m_outputTransitions.count();
}

int ConnectableItem::inputTransitionCount() const
{
    return m_inputTransitions.count();
}
