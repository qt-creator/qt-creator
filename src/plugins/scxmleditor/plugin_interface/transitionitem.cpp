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

#include "transitionitem.h"
#include "connectableitem.h"
#include "cornergrabberitem.h"
#include "finalstateitem.h"
#include "graphicsitemprovider.h"
#include "graphicsscene.h"
#include "parallelitem.h"
#include "sceneutils.h"
#include "scxmldocument.h"
#include "scxmleditorconstants.h"
#include "scxmltagutils.h"
#include "scxmluifactory.h"
#include "serializer.h"
#include "stateitem.h"
#include "tagtextitem.h"

#include <QBrush>
#include <QDebug>
#include <QGraphicsScene>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QUndoStack>
#include <QtMath>

using namespace ScxmlEditor::PluginInterface;

const qreal SELECTION_DISTANCE = 10;

TransitionItem::TransitionItem(BaseItem *parent)
    : BaseItem(parent)
    , m_startTargetFactor(0.5, 0.5)
    , m_endTargetFactor(0.5, 0.5)
{
    setFlag(ItemIsSelectable, true);

    m_highlightPen = QPen(QColor(0xff, 0x00, 0x60));
    m_highlightPen.setWidth(8);
    m_highlightPen.setJoinStyle(Qt::MiterJoin);

    m_pen = QPen(QColor(0x12, 0x12, 0x12));
    m_pen.setWidth(2);

    m_arrow << QPointF(0, 0)
            << QPointF(1, 1)
            << QPointF(0, 1);

    m_eventTagItem = new TagTextItem(this);
    connect(m_eventTagItem, &TagTextItem::selected, this, [=](bool sel){
        setItemSelected(sel);
    });
    connect(m_eventTagItem, &TagTextItem::textReady, this, &TransitionItem::textHasChanged);
    connect(m_eventTagItem, &TagTextItem::movePointChanged, this, &TransitionItem::textItemPositionChanged);

    checkWarningItems();
}

TransitionItem::~TransitionItem()
{
    setBlockUpdates(true);
    removeTransition(Start);
    removeTransition(End);
    //updateTarget();
}

void TransitionItem::checkWarningItems()
{
    ScxmlUiFactory *uifactory = uiFactory();
    if (uifactory) {
        auto provider = static_cast<GraphicsItemProvider*>(uifactory->object("graphicsItemProvider"));
        if (provider) {
            if (!m_warningItem)
                m_warningItem = static_cast<TransitionWarningItem*>(provider->createWarningItem(Constants::C_STATE_WARNING_TRANSITION, this));
        }
    }
}

void TransitionItem::setTag(ScxmlTag *tag)
{
    BaseItem::setTag(tag);
    if (tag) {
        if (tag->tagType() == InitialTransition)
            m_eventTagItem->setVisible(false);
    }
}

void TransitionItem::textItemPositionChanged()
{
    QPointF p = m_eventTagItem->movePoint();
    QString data;
    if (p.toPoint() != QPoint(0, 0)) {
        Serializer s;
        s.append(p);
        data = s.data();
    }
    setEditorInfo(Constants::C_SCXML_EDITORINFO_MOVEPOINT, data);

    updateComponents();
}

void TransitionItem::textHasChanged(const QString &text)
{
    setTagValue("event", text);
}

void TransitionItem::createGrabbers()
{
    if (m_cornerGrabbers.count() != m_cornerPoints.count()) {
        int selectedGrabberIndex = m_cornerGrabbers.indexOf(m_selectedCornerGrabber);

        if (m_cornerGrabbers.count() > 0) {
            qDeleteAll(m_cornerGrabbers);
            m_cornerGrabbers.clear();
        }

        for (int i = 0; i < m_cornerPoints.count(); ++i) {
            auto cornerGrabber = new CornerGrabberItem(this);
            cornerGrabber->setGrabberType(CornerGrabberItem::Circle);
            m_cornerGrabbers << cornerGrabber;
        }

        if (selectedGrabberIndex >= 0 && selectedGrabberIndex < m_cornerGrabbers.count())
            m_selectedCornerGrabber = m_cornerGrabbers[selectedGrabberIndex];
        else
            m_selectedCornerGrabber = nullptr;
    }

    m_pen.setStyle(Qt::DotLine);

    m_lineSelected = true;
    updateGrabberPositions();
}

void TransitionItem::removeGrabbers()
{
    if (m_cornerGrabbers.count() > 0) {
        qDeleteAll(m_cornerGrabbers);
        m_cornerGrabbers.clear();
    }

    m_lineSelected = false;
    m_pen.setStyle(Qt::SolidLine);
}

void TransitionItem::updateGrabberPositions()
{
    for (int i = 0; i < qMin(m_cornerGrabbers.count(), m_cornerPoints.count()); ++i)
        m_cornerGrabbers[i]->setPos(m_cornerPoints[i]);
}

void TransitionItem::removeUnnecessaryPoints()
{
    if (m_cornerPoints.count() > 2) {
        bool found = true;
        while (found) {
            found = false;
            for (int i = 1; i < (m_cornerPoints.count() - 1); ++i) {
                if (QLineF(m_cornerPoints[i], m_cornerPoints[i + 1]).length() <= 20 || QLineF(m_cornerPoints[i], m_cornerPoints[i - 1]).length() <= 20) {
                    m_cornerPoints.takeAt(i);
                    if (i < m_cornerGrabbers.count())
                        delete m_cornerGrabbers.takeAt(i);
                    found = true;
                    break;
                }
            }
        }
    }
    storeValues();
    updateComponents();
}

QVariant TransitionItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant retValue = BaseItem::itemChange(change, value);

    switch (change) {
    case ItemSelectedChange:
        if (!m_mouseGrabbed) {
            if (value.toBool())
                createGrabbers();
            else
                removeGrabbers();
        }
        break;
    case ItemSceneHasChanged:
        checkWarningItems();
        break;
    default:
        break;
    }

    return retValue;
}

void TransitionItem::snapToAnyPoint(int id, const QPointF &newPoint, int diff)
{
    // Check snap to grid
    bool snappedX = false;
    bool snappedY = false;
    for (int i = 0; i < m_cornerPoints.count(); ++i) {
        if (id != i) {
            if (qAbs(newPoint.x() - m_cornerPoints[i].x()) < diff) {
                m_cornerPoints[id].setX(m_cornerPoints[i].x());
                snappedX = true;
            }
            if (qAbs(newPoint.y() - m_cornerPoints[i].y()) < diff) {
                m_cornerPoints[id].setY(m_cornerPoints[i].y());
                snappedY = true;
            }
        }
    }

    if (!snappedX)
        m_cornerPoints[id].setX(newPoint.x());

    if (!snappedY)
        m_cornerPoints[id].setY(newPoint.y());
}

void TransitionItem::snapPointToPoint(int idSnap, const QPointF &p, int diff)
{
    if (idSnap >= 0 && idSnap < m_cornerPoints.count()) {
        if (qAbs(p.x() - m_cornerPoints[idSnap].x()) < diff)
            m_cornerPoints[idSnap].setX(p.x());
        if (qAbs(p.y() - m_cornerPoints[idSnap].y()) < diff)
            m_cornerPoints[idSnap].setY(p.y());
    }
}

void TransitionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // We want to pan scene when Shift is pressed -> that's why ignore mouse-event here
    if (event->modifiers() & Qt::ShiftModifier) {
        event->ignore();
        return;
    }

    QPointF p = event->pos();
    bool bLeftButton = event->button() == Qt::LeftButton;
    if (m_mouseGrabbed) {
        if (bLeftButton) {
            m_cornerPoints.append(p);
            snapToAnyPoint(m_cornerPoints.count() - 1, p);

            if (m_cornerGrabbers.count() > 0) {
                auto corner = new CornerGrabberItem(this);
                corner->setGrabberType(CornerGrabberItem::Circle);
                corner->setPos(p);
                m_cornerGrabbers.append(corner);
            }
        }
        event->accept();
    } else {
        // Check QuickTransition
        if (bLeftButton) {
            // If we found QuickTransition-item or CornerGrabber at this point, we must ignore mouse press here
            // So we can press QuickTransition/CornerGrabber item although there is transition lines front of these items
            foreach (QGraphicsItem *item, scene()->items(event->scenePos())) {
                if (item->type() == QuickTransitionType || (item->type() == CornerGrabberType && item->parentItem() != this)) {
                    event->ignore();
                    return;
                }
            }
        }

        // Check selection
        bool sel = m_lineSelected;
        int i;
        int foundPointIndex = -1;
        for (i = 0; i < m_cornerPoints.count(); ++i) {
            if (QLineF(m_cornerPoints[i], p).length() <= SELECTION_DISTANCE) {
                // Is pressed point close enough of the elbow-point
                foundPointIndex = i;
                sel = true;
                break;
            } else {
                if (i < m_cornerPoints.count() - 1) {
                    QLineF line(m_cornerPoints[i], m_cornerPoints[i + 1]);

                    // Is pressed point close enough of the line
                    QPointF intersPoint;
                    QLineF line2(p, p + QPointF(SELECTION_DISTANCE, SELECTION_DISTANCE));
                    line2.setAngle(line.angle() + 90);
                    if (line.intersect(line2, &intersPoint) == QLineF::BoundedIntersection)
                        sel = true;
                    else {
                        line2.setAngle(line.angle() - 90);
                        sel = line.intersect(line2, &intersPoint) == QLineF::BoundedIntersection;
                    }

                    if (sel)
                        break;
                }
            }
        }

        // Create or remove grabbers
        if (sel != m_lineSelected) {
            if (sel)
                createGrabbers();
            else
                removeGrabbers();

            if (foundPointIndex > 0 && foundPointIndex < m_cornerGrabbers.count()) {
                m_selectedCornerGrabber = m_cornerGrabbers[foundPointIndex];
                m_selectedCornerGrabber->setSelected(true);
            }
        } else if (m_lineSelected && bLeftButton) {
            m_cornerPoints.insert((i + 1), p);
            m_selectedCornerGrabber = new CornerGrabberItem(this);
            m_selectedCornerGrabber->setGrabberType(CornerGrabberItem::Circle);
            m_selectedCornerGrabber->setPos(p);
            m_cornerGrabbers.insert((i + 1), m_selectedCornerGrabber);
            m_selectedCornerGrabber->setSelected(true);
        } else if (m_lineSelected && !bLeftButton) {
            showContextMenu(event);
        }

        if (sel)
            event->accept();
        else
            event->ignore();
    }
}

void TransitionItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // We want to pan scene when Shift is pressed -> that's why ignore mouse-event here
    if (event->modifiers() & Qt::ShiftModifier) {
        event->ignore();
        return;
    }

    if (m_mouseGrabbed) {
        setEndPos(event->pos());
        event->ignore();
    } else if (m_selectedCornerGrabber) {
        snapToAnyPoint(m_cornerGrabbers.indexOf(m_selectedCornerGrabber), event->pos());
        updateComponents();
        storeValues();
        BaseItem::mouseMoveEvent(event);
    }
}

void TransitionItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // We want to pan scene when Shift is pressed -> that's why ignore mouse-event here
    if (event->modifiers() & Qt::ShiftModifier) {
        event->ignore();
        return;
    }

    if (m_mouseGrabbed) {
        if (event->button() == Qt::RightButton) {
            connectToTopItem(mapToScene(event->pos()), TransitionItem::End, m_grabbedTargetType);
            setSelected(false);
            tag()->document()->undoStack()->endMacro();
            m_mouseGrabbed = false;
            ungrabMouse();
            storeValues();
        }
        event->accept();
    } else {
        if (event->button() == Qt::LeftButton) {
            if (m_selectedCornerGrabber) {
                m_selectedCornerGrabber = nullptr;
                setSelected(true);
            }
            removeUnnecessaryPoints();
        }
        BaseItem::mouseReleaseEvent(event);
    }
}

void TransitionItem::checkSelectionBeforeContextMenu(QGraphicsSceneMouseEvent *e)
{
    int ind = -1;
    for (int i = 0; i < m_cornerGrabbers.count(); ++i) {
        if (m_cornerGrabbers[i]->isSelected()) {
            ind = i;
            break;
        }
    }
    m_selectedGrabberIndex = ind;
    BaseItem::checkSelectionBeforeContextMenu(e);
}

void TransitionItem::createContextMenu(QMenu *menu)
{
    QVariantMap data;
    if (m_selectedGrabberIndex > 0) {
        data[Constants::C_SCXMLTAG_ACTIONTYPE] = TagUtils::RemovePoint;
        data["cornerIndex"] = m_selectedGrabberIndex;
        menu->addAction(tr("Remove Point"))->setData(data);
    }

    menu->addSeparator();
    BaseItem::createContextMenu(menu);
}

void TransitionItem::selectedMenuAction(const QAction *action)
{
    if (action) {
        QVariantMap data = action->data().toMap();
        int actionType = data.value(Constants::C_SCXMLTAG_ACTIONTYPE, -1).toInt();

        switch (actionType) {
        case TagUtils::RemovePoint: {
            int ind = data.value("cornerIndex", 0).toInt();
            if (ind > 0) {
                delete m_cornerGrabbers.takeAt(ind);
                m_cornerPoints.takeAt(ind);
                updateComponents();
                storeValues();
            }
            break;
        }
        default:
            BaseItem::selectedMenuAction(action);
            break;
        }
    }
}

void TransitionItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        bool bFound = false;
        if (m_cornerGrabbers.count() > 2) {
            for (int i = m_cornerGrabbers.count() - 1; i >= 1; i--) {
                if (m_cornerGrabbers[i]->isSelected()) {
                    delete m_cornerGrabbers.takeAt(i);
                    m_cornerPoints.takeAt(i);
                    bFound = true;
                }
            }
        }
        if (bFound) {
            updateComponents();
            storeValues();
            event->accept();
            return;
        }
    }

    BaseItem::keyPressEvent(event);
}

bool TransitionItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (watched->type() == CornerGrabberType) {
        auto c = qgraphicsitem_cast<CornerGrabberItem*>(watched);
        auto mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
        if (!c || !mouseEvent)
            return BaseItem::sceneEventFilter(watched, event);

        int cid = m_cornerGrabbers.indexOf(c);
        if (mouseEvent->type() == QEvent::GraphicsSceneMouseMove) {
            if (mouseEvent->buttons() & Qt::LeftButton) {
                QPointF movingPoint = c->pressedPoint() - mouseEvent->pos();

                if (cid == 0) {
                    if (!m_movingFirstPoint) {
                        m_movingFirstPoint = true;
                        removeTransition(Start);
                    }
                } else if (cid == (m_cornerPoints.count() - 1)) {
                    if (!m_movingLastPoint) {
                        m_movingLastPoint = true;
                        if (m_endItem)
                            removeTransition(End);
                        else {
                            updateZValue();
                            updateTargetType();
                        }
                    }
                }

                if (cid >= 0 && cid < m_cornerPoints.count())
                    snapToAnyPoint(cid, m_cornerPoints[cid] - movingPoint);

                updateComponents();
            }
            return true;
        } else if (mouseEvent->type() == QEvent::GraphicsSceneMouseRelease) {
            if (mouseEvent->button() == Qt::LeftButton) {
                if (cid == 0 || (cid == m_cornerPoints.count() - 1)) {
                    m_movingFirstPoint = false;
                    m_movingLastPoint = false;
                    connectToTopItem(watched->mapToScene(mouseEvent->pos()), cid == 0 ? Start : End, UnknownType);
                }
                removeUnnecessaryPoints();
            } else
                showContextMenu(mouseEvent);

            storeValues();
            return true;
        }
    }

    return BaseItem::sceneEventFilter(watched, event);
}

void TransitionItem::removeTransition(TransitionPoint p)
{
    // Remove transition from the item
    // Private function. Transition can itself remove connection from the item
    if (p == Start && m_startItem) {
        m_oldStartItem = m_startItem;
        m_startItem->removeOutputTransition(this);
        m_startItem = nullptr;
        updateZValue();
        updateTargetType();
        if (m_oldStartItem)
            m_oldStartItem->updateTransitions();
    } else if (p == End && m_endItem) {
        m_endItem->removeInputTransition(this);
        m_endItem = nullptr;
        updateZValue();
        updateTargetType();
    }
}

void TransitionItem::disconnectItem(ConnectableItem *item)
{
    // Disconnect item, normally called from the ConnectableItem
    if (item == m_startItem)
        removeTransition(Start);
    if (item == m_endItem)
        removeTransition(End);

    updateTarget();
}

void TransitionItem::setStartItem(ConnectableItem *item)
{
    m_oldStartItem = nullptr;
    m_startItem = item;

    if (item) {
        if (tag())
            tag()->document()->changeParent(tag(), m_startItem->tag());
        item->addOutputTransition(this);

        if (m_cornerPoints.isEmpty()) {
            m_cornerPoints << sceneTargetPoint(TransitionPoint::Start);
            m_cornerPoints << sceneTargetPoint(TransitionPoint::End);
        }
    }

    updateZValue();
    updateComponents();
    storeValues();
}

void TransitionItem::startTransitionFrom(ConnectableItem *item, const QPointF &mouseScenePos)
{
    m_oldStartItem = nullptr;
    m_startItem = item;
    m_startItem->addOutputTransition(this);
    m_cornerPoints.clear();
    m_cornerPoints << sceneTargetPoint(TransitionPoint::Start);
    m_cornerPoints << mapFromScene(mouseScenePos);

    createGrabbers();
    updateZValue();
    updateComponents();
    storeValues();
    m_cornerGrabbers.last()->setSelected(true);
}

void TransitionItem::connectToTopItem(const QPointF &pos, TransitionPoint tp, ItemType targetType)
{
    int cornerPoints = m_cornerPoints.count();

    ConnectableItem *parentItem = nullptr;
    ScxmlTag *parentTag = nullptr;
    ScxmlDocument *document = tag()->document();

    snapToAnyPoint(m_cornerPoints.count() - 1, pos);
    QPointF p(m_cornerPoints.last());

    // First try to find parentItem
    QList<QGraphicsItem*> items = scene()->items(p);
    if (items.count() > 0) {
        for (int i = 0; i < items.count(); ++i) {
            ItemType type = ItemType(items[i]->type());
            if ((targetType == UnknownType && type >= FinalStateType) || type >= StateType) {
                auto it = qgraphicsitem_cast<ConnectableItem*>(items[i]);
                if (it) {
                    parentItem = it;
                    parentTag = parentItem->tag();
                    break;
                }
            }
        }
    }

    if (!parentTag && document)
        parentTag = document->rootTag();

    // Connect existing item
    if (targetType == UnknownType) {
        switch (tp) {
        case Start:
            if (parentItem) {
                m_startTargetFactor = calculateTargetFactor(parentItem, pos);
                savePoint(m_startTargetFactor * 100, Constants::C_SCXML_EDITORINFO_STARTTARGETFACTORS);
            }
            setStartItem(parentItem ? parentItem : m_oldStartItem);
            break;
        case End:
            m_endTargetFactor = calculateTargetFactor(parentItem, pos);
            savePoint(m_endTargetFactor * 100, Constants::C_SCXML_EDITORINFO_ENDTARGETFACTORS);
            setEndItem(parentItem);
            break;
        default:
            break;
        }

        setSelected(false);
        if (parentItem)
            parentItem->setSelected(true);
        removeGrabbers();
        if (m_startItem == m_endItem && cornerPoints == 2) {
            setTagValue("type", "internal");
            setEndItem(0);
            m_targetType = InternalNoTarget;
        }

        updateEventName();
        storeValues();
    } else {
        // Create new item and connect to it
        ConnectableItem *newItem = SceneUtils::createItem(targetType, parentItem ? parentItem->mapFromScene(p) : p);
        if (newItem) {
            ScxmlTag *newTag = SceneUtils::createTag(targetType, tag()->document());
            newItem->setTag(newTag);
            newItem->setParentItem(parentItem);
            if (!parentItem)
                scene()->addItem(newItem);

            newItem->addInputTransition(this);
            newItem->updateAttributes();
            newItem->updateEditorInfo();
            newItem->updateUIProperties();

            if (parentItem)
                parentItem->updateUIProperties();

            if (document)
                document->addTag(parentTag, newTag);

            setEndItem(newItem);
            setSelected(false);
            newItem->setSelected(true);
        }

        removeGrabbers();
    }

    updateTargetType();
}

void TransitionItem::setEndPos(const QPointF &endPos, bool snap)
{
    if (m_cornerPoints.count() >= 2) {
        m_cornerPoints.last().setX(endPos.x());
        m_cornerPoints.last().setY(endPos.y());

        if (snap)
            snapToAnyPoint(m_cornerPoints.count() - 1, endPos);
        updateComponents();
        storeValues();
    }
}

void TransitionItem::setEndItem(ConnectableItem *item)
{

    if (item) {
        m_endItem = item;
        item->addInputTransition(this);
        setEndPos(sceneTargetPoint(End), false);

        if (m_cornerPoints.count() > 2)
            snapPointToPoint(m_cornerPoints.count() - 2, m_cornerPoints.last(), 15);
    } else {
        removeTransition(End);
        updateComponents();
        storeValues();
    }

    updateZValue();
    updateTarget();
}

QPointF TransitionItem::loadPoint(const QString &name)
{
    Serializer s;
    QPointF p;
    s.setData(editorInfo(name));
    s.read(p);
    return p;
}

void TransitionItem::savePoint(const QPointF &p, const QString &name)
{
    Serializer s;
    s.append(p);
    setEditorInfo(name, s.data(), true);
}

QPointF TransitionItem::calculateTargetFactor(ConnectableItem *item, const QPointF &pos)
{
    if (item) {
        QRectF r = item->sceneBoundingRect().adjusted(-8, -8, 8, 8);
        QPointF pixelFactorPoint = pos - r.topLeft();
        QPointF normalizedPoint(qBound(0.0, pixelFactorPoint.x() / r.width(), 1.0), qBound(0.0, pixelFactorPoint.y() / r.height(), 1.0));

        // Center point if close enough
        if (qAbs(normalizedPoint.x() - 0.5) < 0.2 && qAbs(normalizedPoint.y() - 0.5) < 0.2)
            return QPointF(0.5, 0.5);

        return normalizedPoint;
    }

    return QPointF(0.5, 0.5);
}

QPointF TransitionItem::sceneTargetPoint(TransitionPoint p)
{
    ConnectableItem *item = nullptr;
    QPointF factorPoint;
    if (p == TransitionPoint::Start) {
        item = m_startItem;
        factorPoint = m_startTargetFactor;
    } else {
        if (m_endItem) {
            item = m_endItem;
            factorPoint = m_endTargetFactor;
        } else {
            item = m_startItem;
            factorPoint = QPointF(0.5, 0.5);
        }
    }

    QRectF r;
    if (item)
        r = item->sceneBoundingRect();

    return r.topLeft() + QPointF(factorPoint.x() * r.width(), factorPoint.y() * r.height());
}

QPointF TransitionItem::findIntersectionPoint(ConnectableItem *item, const QLineF &line, const QPointF &defaultPoint)
{
    // Check circles
    ItemType t = ItemType(item->type());
    if (t >= InitialStateType && t <= HistoryType) {
        QLineF l = item == m_endItem ? line : QLineF(line.p2(), line.p1());
        l.setLength(l.length() - qMin(item->boundingRect().width(), item->boundingRect().height()) * 0.45);
        return l.p2();
    }

    // Find intersection point between line and target item
    QPolygonF itemPolygon = item->polygonShape();
    if (itemPolygon.count() > 0) {
        QPointF intersectPoint;
        QPointF p1 = itemPolygon.at(0) + item->scenePos();
        QPointF p2;
        QLineF checkLine;
        for (int i = 1; i < itemPolygon.count(); ++i) {
            p2 = itemPolygon.at(i) + item->scenePos();
            checkLine = QLineF(p1, p2);
            if (checkLine.intersect(line, &intersectPoint) == QLineF::BoundedIntersection)
                return intersectPoint;
            p1 = p2;
        }
    }

    return defaultPoint;
}

void TransitionItem::updateComponents()
{
    if (m_cornerPoints.count() < 2)
        return;

    // Check if we must move all points together
    bool movePoints = (SceneUtils::isSomeSelected(m_startItem) && SceneUtils::isSomeSelected(m_endItem) && m_cornerPoints.count() > 2) || (m_movingFirstPoint && m_targetType == InternalNoTarget);

    if (!movePoints && !m_mouseGrabbed) {
        // Check the corners which are in the same line if cornerGrabbers are hidden
        if (m_cornerGrabbers.isEmpty() && m_cornerPoints.count() > 2) {
            for (int i = 0; i < m_cornerPoints.count() - 2; ++i) {
                if (m_cornerPoints[i] != m_cornerPoints[i + 1] && m_cornerPoints[i] != m_cornerPoints[i + 2]) {
                    QLineF l1(m_cornerPoints[i], m_cornerPoints[i + 1]);
                    QLineF l2(m_cornerPoints[i], m_cornerPoints[i + 2]);

                    if (qRound(l1.angle()) == qRound(l2.angle())) {
                        m_cornerPoints.takeAt(i + 1);
                        storeValues();
                        break;
                    }
                }
            }
        }
    }

    if ((m_movingFirstPoint || m_movingLastPoint) && !(m_movingFirstPoint && m_targetType == InternalNoTarget))
        movePoints = false;

    // Init first line
    QLineF firstLine(m_cornerPoints[0], m_cornerPoints[1]);
    if (m_startItem) {
        if (m_targetType <= InternalNoTarget) {
            QPointF p = m_startItem->getInternalPosition(this, m_targetType);
            firstLine.setP1(p);
            firstLine.setP2(p + QPointF(20, 0));
            m_cornerPoints[1] = firstLine.p2();
            if (!(m_movingFirstPoint && m_targetType == InternalNoTarget))
                movePoints = false;
        } else {
            firstLine.setP1(sceneTargetPoint(TransitionPoint::Start));
            firstLine.setP1(findIntersectionPoint(m_startItem, firstLine, firstLine.p1()));
        }
    }

    if (movePoints) {
        if (m_movingFirstPoint && m_targetType == InternalNoTarget)
            m_cornerPoints[1] = m_cornerPoints[0] + QPointF(20, 0);
        else {
            QPointF movingPoint = firstLine.p1() - m_cornerPoints[0];
            for (int i = 0; i < m_cornerPoints.count(); ++i)
                m_cornerPoints[i] += movingPoint;

            for (int i = 0; i < m_arrow.count(); ++i)
                m_arrow[i] += movingPoint;
        }
    }

    m_cornerPoints[0] = firstLine.p1();

    // Init last line
    int lastIndex = m_cornerPoints.count() - 1;
    QLineF lastLine(m_cornerPoints[lastIndex - 1], m_cornerPoints[lastIndex]);
    if (m_endItem && m_targetType > InternalNoTarget) {
        lastLine.setP2(sceneTargetPoint(TransitionPoint::End));
        lastLine.setP2(findIntersectionPoint(m_endItem, lastLine, lastLine.p2()));
        m_cornerPoints[lastIndex] = lastLine.p2();
    }

    m_arrowAngle = 0;
    if (m_targetType == InternalSameTarget) {
        m_arrowAngle = M_PI * 0.6;
    } else {
        // Calculate angle of the lastLine and update arrow
        if (lastLine.length() > 0) {
            m_arrowAngle = qAcos(lastLine.dx() / lastLine.length()) + M_PI;
            if (lastLine.dy() >= 0)
                m_arrowAngle = (2 * M_PI) - m_arrowAngle;
        }
    }

    m_arrow[0] = lastLine.p2() + QPointF(qSin(m_arrowAngle + M_PI / 3) * m_arrowSize, qCos(m_arrowAngle + M_PI / 3) * m_arrowSize);
    m_arrow[1] = lastLine.p2();
    m_arrow[2] = lastLine.p2() + QPointF(qSin(m_arrowAngle + M_PI - M_PI / 3) * m_arrowSize, qCos(m_arrowAngle + M_PI - M_PI / 3) * m_arrowSize);

    setItemBoundingRect(m_cornerPoints.boundingRect().normalized().adjusted(-SELECTION_DISTANCE, -SELECTION_DISTANCE,
        SELECTION_DISTANCE, SELECTION_DISTANCE));

    // Set the right place for the name of the transition
    int ind = m_cornerPoints.count() / 2 - 1;
    QLineF nameLine(m_cornerPoints[ind], m_cornerPoints[ind + 1]);
    if (m_targetType <= InternalNoTarget) {
        m_eventTagItem->setPos(m_cornerPoints[1].x() + 6, m_cornerPoints[1].y() - m_eventTagItem->boundingRect().height() / 3);
    } else {
        const qreal w2 = m_eventTagItem->boundingRect().width() / 2;
        QPointF startPos = nameLine.pointAt(0.5);
        QLineF targetLine(startPos, startPos + QPointF(SELECTION_DISTANCE, SELECTION_DISTANCE));
        targetLine.setAngle(nameLine.angle() + 90);

        m_eventTagItem->setPos(targetLine.p2() + m_eventTagItem->movePoint() - QPointF(w2, m_eventTagItem->boundingRect().height() / 2));
    }

    if (m_warningItem)
        m_warningItem->setPos(m_eventTagItem->pos() - QPointF(WARNING_ITEM_SIZE, 0));
    updateGrabberPositions();
    updateZValue();
}

void TransitionItem::grabMouse(ItemType targetType)
{
    m_grabbedTargetType = targetType;
    m_mouseGrabbed = true;
    BaseItem::grabMouse();
}

void TransitionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(m_pen);

    if (m_cornerPoints.count() >= 2) {
        if (m_targetType == InternalSameTarget) {
            QRectF rect(m_cornerPoints[0].x(), m_cornerPoints[0].y() - SELECTION_DISTANCE,
                m_cornerPoints[1].x() - m_cornerPoints[0].x(),
                SELECTION_DISTANCE * 2);
            painter->drawArc(rect, 0, 180 * 16);
        } else {
            if (highlight()) {
                painter->setPen(m_highlightPen);
                painter->drawPolyline(m_cornerPoints);
            }
            painter->setPen(m_pen);
            painter->drawPolyline(m_cornerPoints);
        }
    }

    for (int i = 0; i < m_cornerPoints.count() - 1; ++i)
        painter->drawEllipse(m_cornerPoints[i], 2, 2);

    if (highlight()) {
        painter->setPen(m_highlightPen);
        painter->drawPolyline(m_arrow);
    }

    painter->setPen(m_pen);
    painter->drawPolyline(m_arrow);

    painter->restore();
}

void TransitionItem::updateEditorInfo(bool allChilds)
{
    BaseItem::updateEditorInfo(allChilds);

    const QColor fontColor = editorInfo(Constants::C_SCXML_EDITORINFO_FONTCOLOR);
    m_eventTagItem->setDefaultTextColor(fontColor.isValid() ? fontColor : Qt::black);

    const QColor stateColor = editorInfo(Constants::C_SCXML_EDITORINFO_STATECOLOR);
    m_pen.setColor(stateColor.isValid() ? stateColor : qRgb(0x12, 0x12, 0x12));
}

void TransitionItem::updateTarget()
{
    setTagValue("target", m_endItem ? m_endItem->itemId() : QString());
    if (m_endItem)
        m_endItem->checkInitial(true);
}

void TransitionItem::updateAttributes()
{
    BaseItem::updateAttributes();

    // Find new target
    if (!m_endItem || tagValue("target") != m_endItem->itemId()) {
        if (m_endItem)
            m_endItem->removeInputTransition(this);

        m_endItem = nullptr;
        findEndItem();
        updateTarget();
        updateZValue();
    }

    // Set event-name
    updateEventName();
    updateTargetType();
}

void TransitionItem::init(ScxmlTag *tag, BaseItem *parentItem, bool initChildren, bool blockUpdates)
{
    Q_UNUSED(initChildren)
    setBlockUpdates(blockUpdates);

    setTag(tag);
    setParentItem(parentItem);
    updateEditorInfo();

    if (blockUpdates)
        setBlockUpdates(false);
}

void TransitionItem::readUISpecifiedProperties(const ScxmlTag *tag)
{
    if (tag) {
        if (m_cornerPoints.count() >= 2) {
            while (m_cornerPoints.count() > 2)
                m_cornerPoints.takeAt(1);

            Serializer s;

            QPointF p = loadPoint(Constants::C_SCXML_EDITORINFO_STARTTARGETFACTORS);
            if (p.isNull())
                p = QPointF(50, 50);
            m_startTargetFactor = p / 100;

            p = loadPoint(Constants::C_SCXML_EDITORINFO_ENDTARGETFACTORS);
            if (p.isNull())
                p = QPointF(50, 50);
            m_endTargetFactor = p / 100;

            QString localPointsData = editorInfo(Constants::C_SCXML_EDITORINFO_LOCALGEOMETRY);
            if (!localPointsData.isEmpty()) {
                QPointF startPos = sceneTargetPoint(Start);
                QPolygonF localPoints;
                s.setData(localPointsData);
                s.read(localPoints);
                for (int i = 0; i < localPoints.count(); ++i)
                    m_cornerPoints.insert(i + 1, startPos + localPoints[i]);
            } else {
                QPolygonF scenePoints;
                s.setData(editorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY));
                s.read(scenePoints);
                for (int i = 0; i < scenePoints.count(); ++i)
                    m_cornerPoints.insert(i + 1, scenePoints[i]);
            }

            m_eventTagItem->resetMovePoint(loadPoint(Constants::C_SCXML_EDITORINFO_MOVEPOINT));

            if (m_lineSelected)
                createGrabbers();
            updateComponents();
        }
    }
}

void TransitionItem::finalizeCreation()
{
    bool old = blockUpdates();
    setBlockUpdates(true);

    updateAttributes();

    if (!old)
        setBlockUpdates(false);
}

void TransitionItem::checkVisibility(double scaleFactor)
{
    m_eventTagItem->setVisible(scaleFactor > 0.5);
}

bool TransitionItem::containsScenePoint(const QPointF &p) const
{
    QPointF pp = mapFromScene(p);

    for (int i = 0; i < m_cornerPoints.count() - 1; ++i) {
        QLineF line(m_cornerPoints[i], m_cornerPoints[i + 1]);

        // Is point close enough of the line
        QPointF intersPoint;
        QLineF line2(pp, pp + QPointF(SELECTION_DISTANCE, SELECTION_DISTANCE));
        line2.setAngle(line.angle() + 90);
        if (line.intersect(line2, &intersPoint) == QLineF::BoundedIntersection) {
            return true;
        } else {
            line2.setAngle(line.angle() - 90);
            if (line.intersect(line2, &intersPoint) == QLineF::BoundedIntersection)
                return true;
        }
    }
    return false;
}

void TransitionItem::findEndItem()
{
    QString targetId = tagValue("target");
    if (!m_endItem && !targetId.isEmpty()) {
        QList<QGraphicsItem*> items = scene()->items();
        for (int i = 0; i < items.count(); ++i) {
            if (items[i]->type() >= FinalStateType) {
                auto item = qgraphicsitem_cast<ConnectableItem*>(items[i]);
                if (item && item->itemId() == targetId) {
                    setEndItem(item);
                    break;
                }
            }
        }
    }
}

void TransitionItem::updateEventName()
{
    m_eventTagItem->setText(tagValue("event"));
}

void TransitionItem::storeGeometry(bool block)
{
    if (tag()) {
        if (m_cornerPoints.count() <= 2) {
            setEditorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY, QString(), block);
            setEditorInfo(Constants::C_SCXML_EDITORINFO_LOCALGEOMETRY, QString(), block);
        } else {
            QPolygonF pol = m_cornerPoints;
            pol.takeFirst();
            pol.takeLast();
            Serializer s;
            for (int i = 0; i < pol.count(); ++i) {
                QPointF spos = sceneTargetPoint(Start);
                pol[i].setX(pol[i].x() - spos.x());
                pol[i].setY(pol[i].y() - spos.y());
            }
            s.append(pol);

            setEditorInfo(Constants::C_SCXML_EDITORINFO_LOCALGEOMETRY, s.data(), block);
        }
    }
}

void TransitionItem::storeMovePoint(bool block)
{
    if (m_eventTagItem->movePoint().toPoint() == QPoint(0, 0))
        setEditorInfo(Constants::C_SCXML_EDITORINFO_MOVEPOINT, QString(), block);
    else
        savePoint(m_eventTagItem->movePoint(), Constants::C_SCXML_EDITORINFO_MOVEPOINT);
}

void TransitionItem::storeTargetFactors(bool block)
{
    if (m_startTargetFactor == QPointF(0.5, 0.5))
        setEditorInfo(Constants::C_SCXML_EDITORINFO_STARTTARGETFACTORS, QString(), block);
    else
        savePoint(m_startTargetFactor * 100, Constants::C_SCXML_EDITORINFO_STARTTARGETFACTORS);

    if (m_endTargetFactor == QPointF(0.5, 0.5))
        setEditorInfo(Constants::C_SCXML_EDITORINFO_ENDTARGETFACTORS, QString(), block);
    else
        savePoint(m_endTargetFactor * 100, Constants::C_SCXML_EDITORINFO_ENDTARGETFACTORS);
}

void TransitionItem::storeValues(bool block)
{
    storeGeometry(block);
    storeMovePoint(block);
    storeTargetFactors(block);
}

void TransitionItem::updateUIProperties()
{
    if (tag() != 0 && isActiveScene())
        storeValues();
}

void TransitionItem::updateTargetType()
{
    if (m_movingFirstPoint && m_targetType == InternalNoTarget)
        return;

    TransitionTargetType type = ExternalTarget;

    if (m_startItem != 0 && m_startItem == m_endItem)
        type = InternalSameTarget;
    else if (m_startItem != 0 && !m_endItem) {
        if (m_movingLastPoint) {
            type = ExternalNoTarget;
        } else {
            QRectF srect = m_startItem->sceneBoundingRect();
            if (srect.contains(m_cornerPoints.last()))
                type = InternalNoTarget;
            else
                type = ExternalNoTarget;
        }
    } else {
        type = ExternalTarget;
    }

    if (type <= InternalNoTarget) {
        m_eventTagItem->resetMovePoint();
        m_arrowSize = 6;
        // Remove extra points
        while (m_cornerPoints.count() > 2)
            m_cornerPoints.takeAt(1);
        while (m_cornerGrabbers.count() > 2)
            delete m_cornerGrabbers.takeAt(1);

        // Remove editorinfo.geometry
        setEditorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY, QString(), true);
        setEditorInfo(Constants::C_SCXML_EDITORINFO_LOCALGEOMETRY, QString(), true);
        setEditorInfo(Constants::C_SCXML_EDITORINFO_MOVEPOINT, QString(), true);
        setEditorInfo(Constants::C_SCXML_EDITORINFO_STARTTARGETFACTORS, QString(), true);
        setEditorInfo(Constants::C_SCXML_EDITORINFO_ENDTARGETFACTORS, QString(), true);
    } else {
        m_arrowSize = 10;
    }

    if (m_targetType != type) {
        m_targetType = type;
        if (m_warningItem)
            m_warningItem->check();
        if (m_startItem && !m_startItem->blockUpdates()) {
            m_startItem->updateOutputTransitions();
            if (m_startItem->type() >= StateType)
                static_cast<StateItem*>(m_startItem)->updateBoundingRect();
        }
    }
}

ConnectableItem *TransitionItem::connectedItem(const ConnectableItem *other) const
{
    if (other) {
        if (other == m_startItem)
            return m_endItem;
        else if (other == m_endItem)
            return m_startItem;
    }

    return nullptr;
}

bool TransitionItem::isStartItem(const ConnectableItem *item) const
{
    return m_startItem == item;
}

bool TransitionItem::isEndItem(const ConnectableItem *item) const
{
    return m_endItem == item;
}

bool TransitionItem::hasStartItem() const
{
    return m_startItem != nullptr;
}

bool TransitionItem::hasEndItem() const
{
    return m_endItem != nullptr;
}

void TransitionItem::updateZValue()
{
    setZValue(qMax(m_startItem ? (m_startItem->zValue() + 1) : 1, m_endItem ? (m_endItem->zValue() + 1) : 1));
}

qreal TransitionItem::textWidth() const
{
    return m_eventTagItem->boundingRect().width();
}

QRectF TransitionItem::wholeBoundingRect() const
{
    return boundingRect().united(m_eventTagItem->sceneBoundingRect());
}
